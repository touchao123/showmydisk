# ── Build stage ──
FROM gcc:13-bookworm AS builder

WORKDIR /src
COPY showmydisk.c Makefile ./
RUN make && make install DESTDIR=/out

# ── Runtime stage ──
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libc-bin \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /out/usr/local/bin/showmydisk /usr/local/bin/showmydisk

ENTRYPOINT ["showmydisk"]
CMD ["--help"]
