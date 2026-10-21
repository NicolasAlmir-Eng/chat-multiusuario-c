FROM gcc:13
RUN apt-get update && \
    apt-get install -y --no-install-recommends make net-tools iputils-ping && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN make clean && make
EXPOSE 9999
CMD ["./servidor"]
