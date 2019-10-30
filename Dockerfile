FROM buildpack-deps

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
      python3 \
      valgrind \
      sudo

RUN mkdir setup
COPY *.sh /setup/
RUN /setup/install-prereqs-debian.sh
RUN /setup/install-wiringPi.sh

VOLUME /code
WORKDIR /code

