FROM btrepp/open62541:tiny
ADD . /tmp/uatools
WORKDIR /tmp/uatools/build
RUN apk add --no-cache cmake gcc && rm -rf /var/cache/apk/* && \
    cmake .. && \
    make && \
    make install && \
    apk del gcc cmake

