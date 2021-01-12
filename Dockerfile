FROM v60:base

RUN apt-get install -y rpm

WORKDIR src

RUN ls

RUN mkdir v60
WORKDIR v60
COPY . .

RUN mkdir build
WORKDIR build

RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DV60_EXAMPLES=OFF ..
RUN ninja && ninja install && ninja package