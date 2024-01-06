# This project is all about socket programming in C++

## Contributers

- 2010213066 Seyyit Muhammet Genc
- 2010213002 Berkay Atas
- 2010213029 Abdulhamit Celik
- Hasbiy Robbiy

## Build Instructions

To build this project from the command line, follow these steps:

> **NOTE** : UNIX NOT YET SUPPORTED. THIS WILL NOT GOING TO COMPILE

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
