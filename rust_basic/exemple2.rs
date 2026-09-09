

fn main(){
    // Em Rust, toda memória alocada no heap (strings, vetores) tem um dono.
    // Quando o dono sai do escopo, o compilador libera a memória.
    // Rust faz isso para NÃO tentar liberar a mesma memória duas vezes.

    let s1 = String::from("texto");
    let s2 = s1; // Rust "move" a posse para s2. s1 fica inválido

    // println!("{}", s1); // Isso gera um erro de compilação.
    println!("{}", s2);

    // Abaixo temos o "empréstimo", que é equivalente a passar por referência (&). Porém,
    // com algumas regras: Podemos ter infinitas referências imutáveis(&) OU uma referência mutável (&mut),
    // mas nunca os dois ao mesmo tempo

    let mut s = String::from("texto");

    // Múltiplas leituras são permitidas
    let r1 = &s;
    let r2 = &s;
    println!("{} e {}", r1, r2); // Ok

    // Apenas UMA escrita é permitida
    let r3 = &mut s;
    r3.push_str(" modificado");
    // println!("{} {}", r1, r3); Erro de compilação: Não pode ler e escrever de uma varável "emprestada"(&)

    println!("{}", r3);

   // r2.push_str("bah, deu erro de compilação :( ");
}