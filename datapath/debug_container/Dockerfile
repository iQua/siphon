FROM ubuntu:cosmic

########################################################
# Essential packages for remote debugging and login in
########################################################

RUN apt-get update && apt-get upgrade -y && apt-get install -y \
  apt-utils gcc g++ openssh-server cmake build-essential gdb gdbserver rsync vim \
  && apt-get clean

RUN mkdir /var/run/sshd
RUN echo 'root:root' | chpasswd
RUN sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config

# SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd

ENV NOTVISIBLE "in users profile"
RUN echo "export VISIBLE=now" >> /etc/profile

# 22 for ssh server. 7777 for gdb server.
EXPOSE 22 7777

RUN useradd -ms /bin/bash debugger
RUN echo 'debugger:pwd' | chpasswd

########################################################
# Add custom packages and development environment here
########################################################

# Expose port for connection from peers
EXPOSE 6868/udp
EXPOSE 6868/tcp
# Expose port for the gRPC service
EXPOSE 6869

# Dependencies
RUN apt-get install -y \
  build-essential autoconf libtool iputils-ping \
  curl \
  git \
  libboost-all-dev \
  pkg-config \
  protobuf-compiler-grpc libprotobuf-dev libprotoc-dev \
  libgrpc-dev libgrpc++-dev \
  && apt-get clean

########################################################

CMD ["/usr/sbin/sshd", "-D"]
