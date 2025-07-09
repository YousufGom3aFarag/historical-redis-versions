# Redis historical versions from 2009

A colleague of mine joined Redis a month ago and approached me to ask if there were available historical versions of Redis, of the very early days. He thought that seeing how the code base started would provide insights about the design process that came later, what was already present since the start, what was missing.

So I did a search on my Gmail for emails from me, with attachment, containing the word *Redis*. After digging a bit, I found two tarballs that I sent to friends of mine in the early days, before Redis was ever released, before the days its source code was published in Google Code.

Surprisingly the official Redis repository still contains code from 22th of March 2009, so I guess I managed (with the help of some friend probably) to import the SVN history from Google Code into the GitHub repository, when I did the switch. Still the two versions of Redis I'm publishing here show the interesting change that happend from:

* Version 0.1 (the tarball had such name) I sent 10th of February.
* Version ?? (no version name this time) I sent 25th of February.

You can see both versions in the respective directories.

However, after digging a bit more in my inbox, I found that Redis actually started in 2008. This is an email I found, that had as recipient a software developer called Uwe Klein:

```
Hello,

thank you very much.
Funny enough I never used this code in Jim, it was kinda
a proof of concept that it was possible to implement the
event loop as an extension instead to have it inside the
core.

But... I'm writing a database engine this days that is using
this code (that was modified to become a library) and I
actually have this bug too in the new code.

Thank you very much for fixing it both in Jim and in
Redis (the name of the DB stuff).
```

I was thanking with Uwe about a bugfix in Jim Tcl that had good effects in Redis as well, and this was happening in February 1st of 2008... That's one year before, it is a lot of time. Now I'm seeing that the Redis 0.1 version I sent to my friend the February of one year later had just 2000 lines of code. It means that I actually didn't start the project for one year?

Also, the second tarball has exactly the same date as the [first public post of Redis on Hacker News](https://news.ycombinator.com/item?id=494649). So indeed things started one year later, but now I know that in early 2008 I was thinking about writing this system, maybe wrote a few lines of code and some doc, and put the real efforts only one year later, I guess, when I really needed it for LLOOGG (my startup at the time). Apparently, however, LLOOGG was likely not the real trigger. My feeling is that writing Jim Tcl and having to work for long hours with database systems is what really pushed me into writing a database that looked like an interpreter for commands.

## Version 0.1 and its TODO note

One interesting thing is that version 0.1 had inside already *most* of what made Redis later what it is today, even if this version was very basic, had no fork() based persistence whatsoever, no sets, no sorted sets, nothing, just set, get, del, ...

This is the TODO note on top of the `redis.c` file:

```
 * TODO:
 * - clients timeout
 * - background and foreground save, saveexit
 * - set/get/delete/exists/incr/decr (use long long for integers)
 * - list operations (lappend,rappend,llen,ljoin,lreverse,lrange,lindex)
 * - sort lists, and insert-in-sort. Different sort methods.
 * - set operations (sadd,sdel,sget,sunion,sintersection,ssubtraction)
 * - expire foobar 60
 * - keys pattern
 */
```

Basically there is everything there: the idea of using fork(), expires, list operations (with odd names), sets, even a mention to a kind of list that is sorted (sorted sets!). Also the structure of the Redis object was very similar to what it is today:

```
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
```

And was conceived since the start to hold different values, exactly like a programming language interpreter would look like. So Redis was never meant to be a plain key-value store, but a data structure store since day 0.

## The 0.2 version, after two weeks of work

Let's call the second version 0.2, even if it was not tagged as such. Surprisingly, in this version there is already fork() based persistence and lists. More or less 1200 lines of code more (from ~2500 to ~3700 comments included) if we exclude `picol.c`, that was there since I thought about integrating a scripting language inside Redis, I guess, but then I changed idea, only to do it years later with Lua :D So another idea that was there since the very start.

## Conclusions

Well, it was fun to dig into the source code of the early versions for a couple of minutes. I hope this is kinda interesting for other people that study how systems evolve, especially systems conceived outside the more rigid design schemes of a company, but created in very spontaneous ways.

To me, it was always clear that the initial *kernel* of ideas of the very first release of a system strongly informs and influences all the future. In my YouTube channel and elsewhere I repeated this concept many times: spend a lot of time in your "toy" first version. From it, a lot of things will spawn. Now I'm even more sure that this is a good advice.
