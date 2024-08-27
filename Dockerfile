FROM ubuntu:latest AS builder

LABEL author="aziabatz"
LABEL version="1.0"

ENV APT_GET_UPDATE 2016-03-01
ENV toolchain /toolchain
ENV target "x86_64-elf"
ENV jobs 4
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    build-essential \
    bison \
    flex \
    libgmp3-dev \
    libmpfr-dev \
    binutils \
    gcc \
    g++ \
    nasm \
    make \
    wget \
    libmpc-dev \
    texinfo \
    tree \
    gdb \
    mtools \
    xorriso \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

ADD http://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.gz .
RUN tar xvzf binutils-2.39.tar.gz \
    && cd binutils-2.39 \
    && ./configure --prefix=${toolchain} --target=${target} --disable-nls --disable-werror --with-sysroot \
    && make -j${jobs} \
    && make install \
    && cd .. \
    && rm -rf binutils-2.39 \
    && rm binutils-2.39.tar.gz

ADD http://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.gz .
RUN tar xvzf gcc-12.2.0.tar.gz \
    && mkdir build-gcc \
    && cd build-gcc \
    && ../gcc-12.2.0/configure --prefix=${toolchain} --target=${target} --disable-nls --enable-languages=c,c++ --without-headers --disable-werror \
    && make -j${jobs} all-gcc \
    && make -j${jobs} all-target-libgcc \
    && make install-gcc \
    && make install-target-libgcc \
    && cd .. \
    && rm -rf build-gcc \
    && rm -rf gcc-12.2.0 \
    && rm gcc-12.2.0.tar.gz

# Stage 2 image
FROM ubuntu:latest

LABEL author="aziabatz"
LABEL version="1.0"

ENV toolchain /toolchain
ENV code /src
ENV DEBIAN_FRONTEND=noninteractive

COPY --from=builder ${toolchain} ${toolchain}

RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    build-essential \
    nasm \
    make \
    gdb \
    mtools \
    xorriso \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

ENV PATH="${toolchain}/bin:${PATH}"
WORKDIR ${code}
ENTRYPOINT ["/bin/bash"]
