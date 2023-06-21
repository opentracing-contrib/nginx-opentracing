FROM golang:alpine AS build
WORKDIR /src
ENV CGO_ENABLED=0
RUN apk add --no-cache \
      git \
      ca-certificates \
      tzdata \
 && update-ca-certificates
COPY . .
RUN go build -o /app .

FROM scratch AS bin
ENV OT_COLLECTOR_ADDR="localhost:4317"
COPY --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
COPY --from=build /app /
ENTRYPOINT ["/app"]
