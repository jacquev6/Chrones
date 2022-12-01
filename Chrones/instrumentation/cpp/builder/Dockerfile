# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

FROM ubuntu:20.04

# System packages
RUN set -x \
 && apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends \
      g++ make `# Our build tools` \
      wget ca-certificates cmake `# For Google Test` \
      python3-pip \
 && apt-get clean

# Google Test
RUN set -x \
 && wget https://github.com/google/googletest/archive/release-1.11.0.tar.gz \
 && tar xzf release-1.11.0.tar.gz \
 && cd googletest-release-1.11.0 \
 && mkdir build \
 && cd build \
 && cmake .. \
 && make \
 && make install \
 && cd ../.. \
 && rm -r googletest-release-1.11.0 \
 && rm release-1.11.0.tar.gz

# Python packages
RUN pip3 install \
      cpplint `# Static analysis tools`
