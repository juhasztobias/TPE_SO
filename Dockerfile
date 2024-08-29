FROM agodio/itba-so-multi-platform:3.0 as base
WORKDIR /app

COPY . .
CMD [ "make", "all" ]