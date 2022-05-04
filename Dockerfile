FROM buildpack-deps

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
      valgrind \
      sudo \
      meson \
      cmake \
      libboost-date-time-dev

RUN mkdir setup
COPY *.sh /setup/
RUN /setup/install-prereqs-debian.sh
RUN /setup/install-wiringPi.sh

VOLUME /code
WORKDIR /code
