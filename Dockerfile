FROM debian:stretch

VOLUME ["/common/plugins"]

WORKDIR /app/Lets/Transform

RUN set -ex; apt-get update && apt-get install -y \
  make \
  cmake \
  libc-dev \
  libmpc-dev \
  libgmp-dev \
  libmpfr-dev \
  binutils

COPY entrypoint.sh .

ENTRYPOINT ["sh", "-c", "/app/Lets/Transform/entrypoint.sh"]
