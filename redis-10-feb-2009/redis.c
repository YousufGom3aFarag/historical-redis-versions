/* Redis - REmote DIctionary Server
 * Copyright (C) 2008 Salvatore Sanfilippo antirez at gmail dot com
 * All Rights Reserved */

/* TODO:
 * - clients timeout
 * - background and foreground save, saveexit
 * - set/get/delete/exists/incr/decr (use long long for integers)
 * - list operations (lappend,rappend,llen,ljoin,lreverse,lrange,lindex)
 * - sort lists, and insert-in-sort. Different sort methods.
 * - set operations (sadd,sdel,sget,sunion,sintersection,ssubtraction)
 * - expire foobar 60
 * - keys pattern
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "ae.h"     /* Event driven programming library */
#include "sds.h"    /* Dynamic safe strings */
#include "anet.h"   /* Networking the easy way */
#include "dict.h"   /* Hash tables */
#include "adlist.h" /* Linked lists */

#define REDIS_OK                0
#define REDIS_ERR               -1
#define REDIS_SERVERPORT        6379    /* TCP port */
#define REDIS_CONNTIMEOUT       30      /* seconds */
#define REDIS_QUERYBUF_LEN      1024
#define REDIS_LEFTREAD_INIT     16
#define REDIS_MAX_ARGS          16

#define REDIS_CMD_BULK          1
#define REDIS_CMD_INLINE        0

/* Object types */
#define REDIS_STRING 0
#define REDIS_LIST 1
#define REDIS_SET 2

/*================================= Data types ============================== */

/* With multiplexing we need to take per-clinet state.
 * Clients are taken in a liked list. */
typedef struct redisClient {
    int fd;
    sds querybuf;
    sds argv[REDIS_MAX_ARGS];
    int argc;
    int bulklen;    /* bulk read len. -1 if not in bulk read mode */
    sds replybuf;
    unsigned int sentlen;
    time_t lastinteraction; /* time of the last interaction, used for timeout */
} redisClient;

/* A redis object, that is a type able to hold a string / list / set */
typedef struct redisObject {
    int type;
    void *ptr;
    int refcount;
    union {
        struct {
            int len;
        } list;
    } u;
} robj;

/* Global server state structure */
struct redisServer {
    int port;
    int conntimeout;
    int fd;
    dict *dict;
    list *clients;
    char neterr[ANET_ERR_LEN];
    aeEventLoop *el;
    int verbose;
};

typedef sds redisCommandProc(redisClient *c);
struct redisCommand {
    char *name;
    redisCommandProc *proc;
    int arity;
    int type;
};

/*================================ Prototypes =============================== */

static void freeStringObject(robj *o);
static void freeListObject(robj *o);
static void freeSetObject(robj *o);
static void freeObject(robj *o);
static robj *createObject(int type, void *ptr);

static sds pingCommand(redisClient *c);
static sds echoCommand(redisClient *c);
static sds setCommand(redisClient *c);
static sds getCommand(redisClient *c);
static sds delCommand(redisClient *c);
static sds existsCommand(redisClient *c);

/*================================= Globals ================================= */

/* Global vars */
static struct redisServer server; /* server global state */
static struct redisCommand cmdTable[] = {
    {"ping",pingCommand,1,REDIS_CMD_INLINE},
    {"echo",echoCommand,2,REDIS_CMD_BULK},
    {"set",setCommand,3,REDIS_CMD_BULK},
    {"get",getCommand,2,REDIS_CMD_INLINE},
    {"del",delCommand,2,REDIS_CMD_INLINE},
    {"exists",existsCommand,2,REDIS_CMD_INLINE},
    {"",NULL,0,0}
};

/*====================== Hash table type implementation  ==================== */

/* This is an hash table type that uses the SDS dynamic strings libary as
 * keys and radis objects as values (objects can hold SDS strings,
 * lists, sets). */

static unsigned int sdsDictHashFunction(const void *key) {
    return dictGenHashFunction(key, sdslen((sds)key));
}

static int sdsDictKeyCompare(void *privdata, const void *key1,
        const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = sdslen((sds)key1);
    l2 = sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

static void sdsDictKeyDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    sdsfree(val);
}

static void sdsDictValDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    freeObject(val);
}

dictType sdsDictType = {
    sdsDictHashFunction,       /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    sdsDictKeyCompare,         /* key compare */
    sdsDictKeyDestructor,      /* key destructor */
    sdsDictValDestructor,      /* val destructor */
};

/* ========================= Random utility functions ======================= */

/* Redis generally does not try to recover from out of memory conditions
 * when allocating objects or strings, it is not clear if it will be possible
 * to report this condition to the client since the networking layer itself
 * is based on heap allocation for send buffers, so we simply abort.
 * At least the code will be simpler to read... */
static void oom(const char *msg) {
    fprintf(stderr, "%s: Out of memory\n",msg);
    abort();
}

/* ====================== Redis server networking stuff ===================== */
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    printf("=\n");
    return 1000;
}

static void initServer() {
    server.port = REDIS_SERVERPORT;
    server.conntimeout = REDIS_CONNTIMEOUT;
    server.fd = anetTcpServer(server.neterr, server.port, NULL);
    if (server.fd == -1) {
        fprintf(stderr, "%s\n", server.neterr);
        exit(1);
    }
    server.dict = dictCreate(&sdsDictType,NULL);
    server.clients = listCreate();
    server.el = aeCreateEventLoop();
    if (!server.dict || !server.clients || !server.el)
        oom("server initialization"); /* Fatal OOM */
    server.verbose = 1; 
    aeCreateTimeEvent(server.el, 1000, serverCron, NULL, NULL);
}

static void freeClientArgv(redisClient *c) {
    int j;

    for (j = 0; j < c->argc; j++) {
        sdsfree(c->argv[j]);
        c->argv[j] = NULL;
    }
    c->argc = 0;
}

static void freeClient(redisClient *c) {
    aeDeleteFileEvent(server.el,c->fd,AE_READABLE);
    aeDeleteFileEvent(server.el,c->fd,AE_WRITABLE);
    sdsfree(c->querybuf);
    sdsfree(c->replybuf);
    freeClientArgv(c);
    close(c->fd);
    free(c);
}

static void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    redisClient *c = privdata;
    int nwritten;

    nwritten = write(fd, c->replybuf+c->sentlen,
        sdslen(c->replybuf)-c->sentlen);
    if (nwritten == -1) {
        if (server.verbose)
            fprintf(stderr, "Error writing to client\n");
        freeClient(c);
        return;
    }
    c->sentlen += nwritten;
    if ((unsigned)nwritten == sdslen(c->replybuf)) {
        sdsfree(c->replybuf);
        c->replybuf = sdsempty();
        c->sentlen = 0;
        aeDeleteFileEvent(server.el,c->fd,AE_WRITABLE);
    }
}

static void prepareReply(redisClient *c, sds reply) {
    if (sdslen(c->replybuf) == c->sentlen) {
        sdsfree(c->replybuf);
        c->replybuf = reply;
        c->sentlen = 0;
        if (aeCreateFileEvent(server.el, c->fd, AE_WRITABLE,
            sendReplyToClient, c, NULL) == AE_ERR) {
            freeClient(c);
        }
    } else {
        c->replybuf = sdscatlen(c->replybuf,reply,sdslen(reply));
        sdsfree(reply);
    }
}

#if 0
static void processQuery(redisClient *c, sds q) {
    int len;
    char *n;
    robj *o;

    len = sreprReadLen(q,&n);
    if (len == -1) {
        sdsfree(q);
        return;
    }

    if (server.verbose) printf("Query! (%d) :%s\n", len,q);
    o = srepr2obj(n,NULL);
    if (!o) {
        if (server.verbose) printf("Query parse error!\n");
    } else {
        robj **cmd;
        sds value, err;
        int args;

        if (o->type != REDIS_LIST || o->u.list.len == 0) {
            if (server.verbose)
                printf("command is not a well formed or a zero length list\n");
            goto cleanup;
        }
        cmd = o->ptr;
        args = o->u.list.len;
        if (cmd[0]->type != REDIS_STRING) {
            if (server.verbose)
                printf("command name is not a string\n");
            goto cleanup;
        }
        if (!strcmp(cmd[0]->ptr,"set")) {
            int retval;

            if (server.verbose) printf("SET command\n");
            if (queryCheckArity(c,args,3)) goto cleanup;
            if (cmd[1]->type != REDIS_STRING) {
                if (server.verbose) printf("key argument is not a string\n");
                goto cleanup;
            }
            retval = dictAdd(server.dict,cmd[1]->ptr,cmd[2]);
            if (retval == DICT_ERR) {
                dictReplace(server.dict,cmd[1]->ptr,cmd[2]);
                value = sdsnew("1");
            } else {
                value = sdsnew("0");
                /* Now the key is in the hash entry, don't free it */
                cmd[1]->ptr = NULL;
            }
            /* Now this object is in the hash entry value, increment the
             * reference counter to make sure it will not be freed by
             * freeObject(o) call at the end of this function. */
            cmd[2]->refcount++;
            if (server.verbose) printf("KEY SET\n");
            prepareReply(c,value,sdsempty());
        } else if (!strcmp(cmd[0]->ptr,"get")) {
            dictEntry *de;
            
            if (server.verbose) printf("GET command\n");
            if (queryCheckArity(c,args,2)) goto cleanup;
            if (cmd[1]->type != REDIS_STRING) {
                if (server.verbose) printf("key argument is not a string\n");
                goto cleanup;
            }
            de= dictFind(server.dict,cmd[1]->ptr);
            if (de == NULL) {
                err = sdsnew("key not found");
                value = sdsempty();
            } else {
                value = obj2str(dictGetEntryVal(de));
                err = sdsempty();
            }
            prepareReply(c,value,err);
        } else if (!strcmp(cmd[0]->ptr,"delete")) {
            int retval;
            
            if (server.verbose) printf("DELETE command\n");
            if (queryCheckArity(c,args,2)) goto cleanup;
            if (cmd[1]->type != REDIS_STRING) {
                if (server.verbose) printf("key argument is not a string\n");
                goto cleanup;
            }
            retval = dictDelete(server.dict,cmd[1]->ptr);
            err = sdsempty();
            if (retval == DICT_ERR) {
                value = sdsnew("0");
            } else {
                value = sdsnew("1");
            }
            prepareReply(c,value,err);
        }
    }
cleanup:
    freeObject(o);
    sdsfree(q);
}
#endif

static struct redisCommand *lookupCommand(char *name) {
    int j = 0;
    while(cmdTable[j].name != NULL) {
        if (!strcmp(name,cmdTable[j].name)) return &cmdTable[j];
        j++;
    }
    return NULL;
}

/* resetClient prepare the client to process the next command */
static void resetClient(redisClient *c) {
    freeClientArgv(c);
    c->bulklen = -1;
}

static void processCommand(redisClient *c) {
    struct redisCommand *cmd;

    sdstolower(c->argv[0]);
    /* The QUIT command is handled as a special case. Normal command
     * procs are unable to close the client connection safely */
    if (!strcmp(c->argv[0],"quit")) {
        freeClient(c);
        return;
    }
    // printf("looking up '%s'\n", c->argv[0]);
    cmd = lookupCommand(c->argv[0]);
    if (!cmd) {
        prepareReply(c,sdsnew("-ERR unknown command\r\n"));
        resetClient(c);
        return;
    } else if (cmd->arity != c->argc) {
        prepareReply(c,sdsnew("-ERR wrong number of arguments\r\n"));
        resetClient(c);
        return;
    } else if (cmd->type == REDIS_CMD_BULK && c->bulklen == -1) {
        int bulklen = atoi(c->argv[c->argc-1]);

        sdsfree(c->argv[c->argc-1]);
        if (bulklen == 0) {
            c->argv[c->argc-1] = sdsempty();
        } else {
            if (bulklen < 0 || bulklen > 1024*1024) {
                prepareReply(c,sdsnew("-ERR invalid bulk write count\r\n"));
                resetClient(c);
                return;
            }
            c->argv[c->argc-1] = NULL;
            c->argc--;
            c->bulklen = bulklen+2; /* add two bytes for CR+LF */
            /* It is possible that the bulk read is already in the
             * buffer. Check this condition and handle it accordingly */
            if (sdslen(c->querybuf) >= c->bulklen) {
                c->argv[c->argc] = sdsnewlen(c->querybuf,c->bulklen-2);
                c->argc++;
                c->querybuf = sdsrange(c->querybuf,c->bulklen+2,-1);
            } else {
                return;
            }
        }
    }
    /* Exec the command */
    prepareReply(c,cmd->proc(c));
    resetClient(c);
}

static void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    redisClient *c = (redisClient*) privdata;
    char buf[REDIS_QUERYBUF_LEN];
    int nread;

    nread = read(fd, buf, REDIS_QUERYBUF_LEN);
    if (nread == -1) {
        if (server.verbose)
            fprintf(stderr, "Reading from client: %s\n",strerror(errno));
        freeClient(c);
        return;
    } else if (nread == 0) {
        if (server.verbose) fprintf(stderr, "Client closed connection\n");
        freeClient(c);
        return;
    }
    if (nread) c->querybuf = sdscatlen(c->querybuf, buf, nread);

    if (c->bulklen == -1) {
        /* Read the first line of the query */
        char *p = strchr(c->querybuf,'\n');
        size_t querylen;
        if (p) {
            sds query, *argv;
            int argc, j;
            
            query = c->querybuf;
            c->querybuf = sdsempty();
            querylen = 1+(p-(query));
            if (sdslen(query) > querylen) {
                /* leave data after the first line of the query in the buffer */
                c->querybuf = sdscatlen(c->querybuf,query+querylen,sdslen(query)-querylen);
            }
            *p = '\0'; /* remove "\n" */
            if (*(p-1) == '\r') *(p-1) = '\0'; /* and "\r" if any */
            sdsupdatelen(query);

            /* Now we can split the query in arguments */
            if (sdslen(query) == 0) {
                /* Ignore empty query */
                sdsfree(query);
                return;
            }
            argv = sdssplitlen(query,sdslen(query)," ",1,&argc);
            sdsfree(query);
            if (argv == NULL) oom("Splitting query in token");
            for (j = 0; j < argc && j < REDIS_MAX_ARGS; j++) {
                if (sdslen(argv[j])) {
                    c->argv[c->argc] = argv[j];
                    c->argc++;
                } else {
                    sdsfree(argv[j]);
                }
            }
            free(argv);
            processCommand(c);
            return;
        } else if (sdslen(c->querybuf) >= 1024) {
            if (server.verbose) fprintf(stderr, "Client protocol error\n");
            freeClient(c);
            return;
        }
    } else {
        /* Bulk read handling. Note that if we are at this point
           the client already sent a command terminated with a newline,
           we are reading the bulk data that is actually the last
           argument of the command. */
        int qbl = sdslen(c->querybuf);

        if (c->bulklen <= qbl) {
            /* Copy everything but the final CRLF as final argument */
            c->argv[c->argc] = sdsnewlen(c->querybuf,c->bulklen-2);
            c->argc++;
            c->querybuf = sdsrange(c->querybuf,c->bulklen+2,-1);
            processCommand(c);
            return;
        }
    }
}

static int createClient(int fd) {
    redisClient *c = malloc(sizeof(*c));

    anetNonBlock(NULL,fd);
    if (!c) return REDIS_ERR;
    c->fd = fd;
    c->querybuf = sdsempty();
    c->argc = 0;
    c->bulklen = -1;
    c->replybuf = sdsempty();
    c->sentlen = 0;
    c->lastinteraction = time(NULL);
    if (!c->querybuf || !c->replybuf) {
        freeClient(c);
        return REDIS_ERR;
    }
    if (aeCreateFileEvent(server.el, c->fd, AE_READABLE,
        readQueryFromClient, c, NULL) == AE_ERR) {
        freeClient(c);
        return REDIS_ERR;
    }
    return REDIS_OK;
}

static void acceptHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd;
    char cip[128];
    redisClient *client;

    cfd = anetAccept(server.neterr, fd, cip, &cport);
    if (cfd == AE_ERR) {
        if (server.verbose) printf("Warning: Accept error %s\n", server.neterr);
        return;
    }
    if (server.verbose) printf("Accepted %s:%d\n", cip, cport);
    if (createClient(cfd) == REDIS_ERR) {
        close(cfd); /* May be already closed, just ingore errors */
        return;
    }
}

/* ======================= Redis objects implementation ===================== */
static robj *createObject(int type, void *ptr) {
    robj *o = malloc(sizeof(*o));

    if (!o) oom("createObject out of memory");
    o->type = type;
    o->ptr = ptr;
    o->refcount = 1;
    return o;
}

static void freeStringObject(robj *o) {
    sdsfree(o->ptr);
    free(o);
}

static void freeListObject(robj *o) {
    int i;

    robj **v = o->ptr;
    for(i = 0; i < o->u.list.len; i++)
        freeObject(v[i]);
    free(v);
    free(o);
}

static void freeSetObject(robj *o) {
    /* TODO */
    free(o);
}

static void freeObject(robj *o) {
    if (--o->refcount == 0) {
        switch(o->type) {
        case REDIS_STRING: freeStringObject(o); break;
        case REDIS_LIST: freeListObject(o); break;
        case REDIS_SET: freeSetObject(o); break;
        default: assert(0 != 0); break;
        }
    }
}

/*================================== Commands =============================== */

static sds pingCommand(redisClient *c) {
    return sdsnew("+PONG\r\n");
}

static sds echoCommand(redisClient *c) {
    sds echobuf;

    echobuf = sdsempty();
    echobuf = sdscatprintf(echobuf,"%d\r\n",(int)sdslen(c->argv[1]));
    echobuf = sdscatlen(echobuf,c->argv[1],sdslen(c->argv[1]));
    echobuf = sdscatlen(echobuf,"\r\n",2);
    sdsfree(c->argv[1]);
    c->argv[1] = sdsempty();
    return echobuf;
}

static sds setCommand(redisClient *c) {
    int retval;
    robj *o;

    o = createObject(REDIS_STRING,c->argv[2]);
    c->argv[2] = NULL;
    retval = dictAdd(server.dict,c->argv[1],o);
    if (retval == DICT_ERR) {
        dictReplace(server.dict,c->argv[1],o);
    } else {
        /* Now the key is in the hash entry, don't free it */
        c->argv[1] = NULL;
    }
    return sdsnew("+OK\r\n");
}

static sds getCommand(redisClient *c) {
    dictEntry *de;
    sds reply;
    
    de = dictFind(server.dict,c->argv[1]);
    if (de == NULL) {
        reply = sdsnew("0\r\n\r\n");
    } else {
        robj *o = dictGetEntryVal(de);
        sds value;
        
        if (o->type != REDIS_STRING) {
            char *err = "GET against key not holding a string value";
            reply = sdsempty();
            reply = sdscatprintf(reply,"%d\r\n%s\r\n",-((int)strlen(err)),err);
        } else {
            value = o->ptr;
            reply = sdsempty();
            reply = sdscatprintf(reply,"%d\r\n",(int)sdslen(value));
            reply = sdscatlen(reply,value,sdslen(value));
            reply = sdscatlen(reply,"\r\n",2);
        }
    }
    return reply;
}

static sds delCommand(redisClient *c) {

    dictDelete(server.dict,c->argv[1]);
    return sdsnew("+OK\r\n");
}

static sds existsCommand(redisClient *c) {
    dictEntry *de;
    sds reply;
    
    de = dictFind(server.dict,c->argv[1]);
    if (de == NULL)
        reply = sdsnew("0\r\n");
    else
        reply = sdsnew("1\r\n");
    return reply;
}

/* =================================== Main! ================================ */

int main(int argc, char **argv) {
    initServer();
    if (aeCreateFileEvent(server.el, server.fd, AE_READABLE,
        acceptHandler, NULL, NULL) == AE_ERR) oom("creating file event");
    aeMain(server.el);
    aeDeleteEventLoop(server.el);
    return 0;
}
