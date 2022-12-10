# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

FROM ubuntu:22.04

RUN set -x \
 && apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends \
      python3 python3-pip python3-venv \
      git ssh \
 && apt-get clean

ADD requirements.txt .

RUN pip3 install -r requirements.txt

ARG UID
RUN useradd user --uid $UID
USER user
