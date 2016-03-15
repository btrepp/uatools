FROM btrepp/open62541
ADD . /tmp/uatools
WORKDIR /tmp/uatools
RUN gcc -o uaconnect uaconnect.c  -lopen62541

