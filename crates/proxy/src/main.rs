use std::net::SocketAddr;

use axum::{
    body::Body,
    extract::ConnectInfo,
    handler::HandlerWithoutStateExt,
    http::{HeaderValue, Request},
    response::Response,
};
use hyper::{
    StatusCode, Uri,
    header::{self},
};
use hyper_util::rt::TokioIo;
use tokio::net::TcpStream;
use tracing::{error, info};

const PROXY_ADDR: &str = "0.0.0.0:8080";
const UPSTREAM_ADDR: &str = "localhost:8081";
const UPSTREAM_HOST: &str = "localhost:8081";

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt::init();

    let listener = tokio::net::TcpListener::bind(PROXY_ADDR).await.unwrap();

    info!(addr = %PROXY_ADDR, "proxy server listening");

    axum::serve(
        listener,
        handler.into_make_service_with_connect_info::<SocketAddr>(),
    )
    .await
    .unwrap();
}

async fn handler(
    connect_info: ConnectInfo<SocketAddr>,
    mut req: Request<Body>,
) -> Result<Response, StatusCode> {
    log_req(&req);

    rewrite_request(&mut req, &connect_info)?;

    let res = forward_request(req).await?;

    Ok(res)
}

fn log_req(req: &Request<Body>) {
    info!(
        method = %req.method(),
        uri = %req.uri(),
        headers = ?req.headers(),
        "received request"
    );
}

fn rewrite_request(
    req: &mut Request<Body>,
    connect_info: &ConnectInfo<SocketAddr>,
) -> Result<(), StatusCode> {
    let path_and_query = req
        .uri()
        .path_and_query()
        .map(|pq| pq.as_str())
        .unwrap_or("/");

    let upstream_uri = path_and_query
        .parse::<Uri>()
        .map_err(|_| StatusCode::BAD_GATEWAY)?;

    println!("{}", upstream_uri);

    *req.uri_mut() = upstream_uri;

    req.headers_mut()
        .insert(header::HOST, HeaderValue::from_static(UPSTREAM_HOST));

    let client_ip = connect_info.0.ip().to_string();

    req.headers_mut().insert(
        "x-forward-for",
        HeaderValue::from_str(&client_ip).map_err(|_| StatusCode::BAD_GATEWAY)?,
    );

    Ok(())
}

async fn forward_request(req: Request<Body>) -> Result<Response, StatusCode> {
    let stream = TcpStream::connect(UPSTREAM_ADDR)
        .await
        .map_err(|_| StatusCode::BAD_GATEWAY)?;

    let io = TokioIo::new(stream);

    let (mut sender, conn) = hyper::client::conn::http1::handshake::<TokioIo<TcpStream>, Body>(io)
        .await
        .map_err(|_| StatusCode::BAD_GATEWAY)?;

    tokio::spawn(async move {
        if let Err(err) = conn.await {
            error!(error = ?err, "upstream connection failed");
        }
    });

    info!("forwarding request upstream");

    let res = sender
        .send_request(req)
        .await
        .map_err(|_| StatusCode::BAD_GATEWAY)?;

    Ok(res.map(Body::new))
}
