FROM ubuntu:latest

RUN apt update -y && apt upgrade -y
RUN apt install -y build-essential git iproute2 iputils-ping netcat-openbsd
