
# official GCC image
FROM gcc:13.2.0

# set the working directory to /app
WORKDIR /app

# update Linux dependencies and install CMake 
RUN apt update && \
    apt install -y cmake

# copy everything into the container at our /app except the files specified in .dockerignore
COPY . .

# create build directory
RUN mkdir build

# change directory to build
WORKDIR /app/build

# run cmake
RUN cmake ../ -G "Unix Makefiles"

# run make
CMD ["make"]
