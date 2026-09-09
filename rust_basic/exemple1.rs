use std::io;

fn main(){
    // Toda variável em Rust é IMUTÁVEL por padrão.

    let x = 5; // Essa variável é imutável 
    // x = 7; ERRO

    // Para uma variável ser mutável, temos que dizer explicitamente para o compilador 
    
    let mut y = 10; // 'mut' avisa que essa variável pode ser alterada
    y = 11;

    println!("x: {}, y: {}", x, y);
}