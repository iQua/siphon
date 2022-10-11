FROM ubuntu:18.04

LABEL "maintainer"="Baochun Li <bli@ece.toronto.edu>"

# Expose port for connection from peers
EXPOSE 6699

# Configuring the timezone
ENV TZ=America/Toronto
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Dependencies
RUN apt-get update && apt-get install -y \
  redis-server=5:4.0.9-1ubuntu0.2 \
  redis=5:4.0.9-1ubuntu0.2 \
  python3-pip \
  net-tools \
  inetutils-ping \
  curl \
  nodejs=8.10.0~dfsg-2ubuntu0.4 \
  vim \
  npm=3.5.2-0ubuntu4 \
  && apt-get clean

# Stopping redis-server
RUN service redis-server stop

# Build
COPY . /var/siphon_controller

# Running server
WORKDIR /var/siphon_controller/src

RUN npm install ioredis@3.2.2

# CMD ["node","server.js","static_2N_udp_routes.json"]
