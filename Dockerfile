FROM btrepp/open62541
RUN apk add --no-cache gdb  && rm -rf /var/cache/apk/*
ADD . /tmp/uatools
WORKDIR /tmp/uatools/build
RUN cmake ..
RUN make && make install

