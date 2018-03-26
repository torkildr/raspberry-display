#!/bin/bash

tag="raspberry-display"

if id -nG "$USER" | grep -qw "docker"; then
  docker="docker"
else
  docker="sudo docker"
fi

$docker build --tag "$tag" .
$docker run -it -v $(realpath .):/code $tag

