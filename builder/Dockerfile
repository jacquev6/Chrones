FROM debian:10

RUN set -x \
 && apt-get update \
 && apt-get install --yes \
      wget build-essential cmake \
      python3-pip \
 && pip3 install cpplint \
 && rm -rf /var/lib/apt/lists/*

RUN set -x \
 && wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz \
 && tar xzf release-1.10.0.tar.gz \
 && cd googletest-release-1.10.0 \
 && mkdir build \
 && cd build \
 && cmake .. \
 && make \
 && make install \
 && cd ../.. \
 && rm -r googletest-release-1.10.0 \
 && rm release-1.10.0.tar.gz \
 `# Customize Google Test to display more elements in STL containers when assertions fail` \
 && sed -i 's/const size_t kMaxCount = 32;/const size_t kMaxCount = 1024;/' /usr/local/include/gtest/gtest-printers.h

RUN set -x \
 && wget https://github.com/lindenb/makefile2graph/archive/656a5a996e2aa70db8c9f0bb506003a24f3666cb.tar.gz \
 && tar xzf 656a5a996e2aa70db8c9f0bb506003a24f3666cb.tar.gz \
 && cd makefile2graph-656a5a996e2aa70db8c9f0bb506003a24f3666cb \
 && make \
 && make install \
 && cd .. \
 && rm -r makefile2graph-656a5a996e2aa70db8c9f0bb506003a24f3666cb \
 && rm 656a5a996e2aa70db8c9f0bb506003a24f3666cb.tar.gz
