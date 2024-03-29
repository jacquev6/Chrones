# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

FROM ubuntu:22.04

# System packages
RUN set -x \
 && apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends \
      g++ make `# Build tools` \
      wget ca-certificates cmake `# For Google Test` \
      software-properties-common dirmngr gpg-agent `# Required when adding the NVidia APT repository` \
      python3 python3-pip python3-venv \
      git ssh \
      jq \
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

# NVidia packages
RUN set -x \
 && apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/3bf863cc.pub \
 && add-apt-repository "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/ /" \
 && apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends \
      cuda-nvcc-11-7 `# Build tools` \
      cuda-cudart-dev-11-7 `# Direct dependencies` \
 && apt-get clean
ENV PATH=/usr/local/cuda-11.7/bin:$PATH

# Python packages
RUN pip3 install \
      cpplint `# Static analysis` \
      semver build twine `# Publication`

# User
ARG UID
RUN useradd user --uid $UID --create-home
USER user
ENV PATH=/home/user/.local/bin:$PATH

# Python packages in user mode, just to speed-up the development cycle
RUN pip3 install \
      click matplotlib psutil dacite
