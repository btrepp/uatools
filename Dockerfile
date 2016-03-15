FROM btrepp/open62541
ADD . /tmp/uatools
WORKDIR /tmp/uatools/build
RUN cmake ..
RUN make && make install

