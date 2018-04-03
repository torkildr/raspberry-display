#!/bin/bash

tag="raspberry-display"

if id -nG "$USER" | grep -qw "docker"; then
  docker="docker"
else
  docker="sudo docker"
fi

current_dir="$PWD/${1#./}"

$docker build --tag "$tag" .
$docker run -it -v "${current_dir}:/code" $tag

