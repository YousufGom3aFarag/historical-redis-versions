# Redis Through the Ages: Explore Historical Versions of Redis 🚀

![Redis Logo](https://redis.io/images/redis-white.png)

Welcome to the **historical-redis-versions** repository! Here, you can dive into the early versions of Redis and discover the backstory behind its evolution. This repository is a treasure trove for developers, historians, and Redis enthusiasts alike.

## Table of Contents

- [About](#about)
- [Historical Versions](#historical-versions)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)
- [Links](#links)

## About

Redis is an in-memory data structure store, often used as a database, cache, and message broker. Its journey began in 2009, and since then, it has grown into a widely-used technology. This repository focuses on the earlier versions of Redis, allowing you to see how it has changed over the years.

### Why Explore Historical Versions?

- **Understand Evolution**: Learn how Redis has evolved to meet the demands of developers.
- **Debugging**: If you encounter issues with modern versions, knowing the history can help you understand the changes.
- **Educational**: Great for those who want to learn more about the development of open-source software.

## Historical Versions

In this section, you will find various early versions of Redis. Each version comes with its own set of features and limitations. Below is a brief overview of some key versions:

### Redis 1.0

- **Release Date**: May 2009
- **Key Features**:
  - Basic data structures: Strings, Lists, Sets
  - Simple command structure
- **Backstory**: This was the first official release of Redis. It laid the groundwork for future enhancements.

### Redis 2.0

- **Release Date**: April 2010
- **Key Features**:
  - Introduction of Pub/Sub messaging
  - Enhanced performance
- **Backstory**: The addition of Pub/Sub allowed Redis to be used for real-time messaging applications.

### Redis 2.2

- **Release Date**: March 2011
- **Key Features**:
  - Support for sorted sets
  - Improved replication
- **Backstory**: This version made Redis more versatile, allowing developers to build more complex applications.

### Redis 3.0

- **Release Date**: June 2015
- **Key Features**:
  - Introduction of Redis Cluster
  - Enhanced memory efficiency
- **Backstory**: Redis Cluster was a significant step toward scalability, allowing for sharding and better data distribution.

### Redis 4.0

- **Release Date**: July 2017
- **Key Features**:
  - Modules support
  - Improved performance for large datasets
- **Backstory**: This version opened the door for custom extensions, allowing developers to tailor Redis to their specific needs.

## Installation

To install any of the historical versions, follow these steps:

1. **Download the Version**: Visit the [Releases section](https://github.com/YousufGom3aFarag/historical-redis-versions/releases) to find the version you want. Click on the appropriate link to download it.

2. **Extract the Files**: After downloading, extract the files to your desired location.

3. **Build Redis**: Navigate to the directory where you extracted the files and run the following command:

   ```bash
   make
   ```

4. **Run Redis**: Start the Redis server using the command:

   ```bash
   src/redis-server
   ```

5. **Verify Installation**: You can verify that Redis is running by executing:

   ```bash
   src/redis-cli ping
   ```

   If everything is set up correctly, you should receive a response of `PONG`.

## Usage

Using Redis is straightforward. Here are some basic commands to get you started:

### Basic Commands

- **Set a Key**: 
  ```bash
  SET mykey "Hello"
  ```

- **Get a Key**: 
  ```bash
  GET mykey
  ```

- **Delete a Key**: 
  ```bash
  DEL mykey
  ```

### Working with Lists

- **Push to a List**: 
  ```bash
  LPUSH mylist "World"
  ```

- **Retrieve from a List**: 
  ```bash
  LRANGE mylist 0 -1
  ```

### Working with Sets

- **Add to a Set**: 
  ```bash
  SADD myset "Hello"
  ```

- **Check Membership**: 
  ```bash
  SISMEMBER myset "Hello"
  ```

### Working with Sorted Sets

- **Add to a Sorted Set**: 
  ```bash
  ZADD myzset 1 "one"
  ```

- **Retrieve from a Sorted Set**: 
  ```bash
  ZRANGE myzset 0 -1
  ```

## Contributing

Contributions are welcome! If you have any early versions of Redis that are not included in this repository, please follow these steps:

1. **Fork the Repository**: Click on the "Fork" button at the top right of this page.

2. **Create a New Branch**: 
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make Your Changes**: Add your historical version files and any relevant documentation.

4. **Commit Your Changes**: 
   ```bash
   git commit -m "Add historical version X.X"
   ```

5. **Push to Your Fork**: 
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Create a Pull Request**: Go to the original repository and click on "New Pull Request."

## License

This repository is licensed under the MIT License. Feel free to use and modify it as you see fit.

## Links

For downloading the historical versions, visit the [Releases section](https://github.com/YousufGom3aFarag/historical-redis-versions/releases). You can find all the available versions there, ready for you to explore.

![Download Button](https://img.shields.io/badge/Download%20Releases-Click%20Here-blue)

Feel free to check back regularly for updates and new historical versions. Happy exploring!