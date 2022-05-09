#!/bin/bash

set -e

tag="raspberry-display"

if $(id -nG "$USER" | grep -qw "docker") || [[ $OSTYPE == 'darwin'* ]]; then
  docker="docker"
else
  docker="sudo docker"
fi

current_dir="$PWD/${1#./}"

$docker build --tag "$tag" .
$docker run -p 8080 -it -v "${current_dir}:/code" $tag

