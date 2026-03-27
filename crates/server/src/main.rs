use axum::{
    Router,
    body::Body,
    extract::Request,
    http::{StatusCode, Uri},
    routing::get,
};
use tracing::info;
use tracing_subscriber;

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt::init();

    let app = Router::new()
        .route("/", get(handler))
        .fallback(fallback_handler);

    let addr = "0.0.0.0:8081";
    let listener = tokio::net::TcpListener::bind(addr).await.unwrap();

    info!(%addr, "origin server listening");

    axum::serve(listener, app).await.unwrap();
}

fn handler(req: Request<Body>) -> impl Future<Output = &'static str> {
    async {
        info!("received request at /");

        let (parts, _) = req.into_parts();

        println!("method: {}", parts.method);
        println!("uri: {}", parts.uri);
        println!("headers: {:#?}", parts.headers);

        "origin server response"
    }
}

async fn fallback_handler(uri: Uri) -> (StatusCode, String) {
    (StatusCode::NOT_FOUND, format!("no route for {uri}"))
}
