FROM alpine:latest

RUN apk --update --no-cache add autoconf g++ make git bison re2c libxml2-dev sqlite-dev pcre-dev && \
    git clone https://github.com/php/php-src.git ./php-src && \
    cd ./php-src && git checkout tags/php-7.3.6 && \
    ./buildconf --force && ./configure --prefix=/usr/local/php7 && make -j4 && make install

WORKDIR /src