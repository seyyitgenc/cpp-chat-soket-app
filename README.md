# This projects is all about socket programming in C++

## Build Instructions

To build this project from the command line, follow these steps:

```sh
# 1. Navigate to the project directory. if you alread in project directory you  can ignore this:

    cd /path/to/project

# 2. Create a build directiory and navigate into it:

    mkdir build
    cd build

# 3. Run CMake to generate the build files :

    cmake .. -G "Unix Makefiles"

# 4. Build the project:

    make
```

---

## Running with Docker

Build and Run the docker image:

```sh
docker compose up --build
```

Thats it! now you can connect to the server with your favorite client.
