FROM buildpack-deps

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
      valgrind \
      sudo \
      libmosquitto-dev \
      mosquitto \
      mosquitto-clients \
      colormake \
      bear \
      clangd \
      cmake

RUN mkdir setup
COPY *.sh /setup/
RUN /setup/install-prereqs-debian.sh
RUN /setup/install-wiringPi.sh
RUN echo 'alias make="colormake"' >> ~/.profile

RUN cd /setup \
    && git clone https://github.com/catchorg/Catch2.git \
    && cd Catch2 \
    && cmake -B build -S . -DBUILD_TESTING=OFF \
    && cmake --build build/ --target install
VOLUME /code
WORKDIR /code
