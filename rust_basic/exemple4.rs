// 'usize' é um inteiro sem sinal. Equivalente a 'size_t'
// A função retorna um tipo 'Result': Se der certo retorna um inteiro 'i32',
// se der erro, retorna uma String
fn buscar_elemento(indice: usize) -> Result<i32, String>{
    let array = [10, 20, 30, 40];

    if indice < array.len(){
        // 'Ok' indica sucesso
        Ok(array[indice]) // retorna o índice solicitado
    } else{
        // Retorna uma String com uma mensagem de Erro
        Err(format!("Índice({}) inválido.", indice)) // o 'format!' serve para usar um parâmentro na string, como em um print
    }
}

fn main(){
    // let indice1 = 5;
    let indice2 = 3;

    // Usamos o 'match' para verificar o Result. O match é equivalente ao switch case
    match buscar_elemento(indice2){
        // Se a busca deu certo, o número é armazenado em 'valor' e usado no print
        Ok(valor) => {
            println!("Elemento encontrado: {}", valor);
        },
        Err(mensagem_erro) => {
            println!("Erro na busca: {}", mensagem_erro);
        }
    }
}