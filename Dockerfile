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
      clangd

RUN mkdir setup
COPY *.sh /setup/
RUN /setup/install-prereqs-debian.sh
RUN /setup/install-wiringPi.sh
RUN echo 'alias make="colormake"' >> ~/.bashrc

VOLUME /code
WORKDIR /code
