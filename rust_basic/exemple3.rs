/*
    A parte de POO também é um pouco diferente do C++. Em Rust não tem a palavra 'class'
    e nem o conceito de herança. Em Rust temos uma sepração definitiva entre dados e 
    comportamento (métodos)

    Abaixo vou fazer o esqueleto da lógica do Snake Game para exemplificar
*/

// A 'struct' armazena apenas os atributos e por padrão todos os campos são 
// privados. Usamos 'pub' para deixar público
pub struct Cobra{
    tamanho: u32, // 'u32' é um número inteiro (32 bits = 4 bytes)
    viva: bool,
}

// Para fazer a implementação (métodos) usamos um bloco de implementação (impl)
impl Cobra{
    // Construtor é uma função que retorna uma instânia da struct (self)
    pub fn new(tamanho_inicial: u32) -> Self{
        Self{
            tamanho: tamanho_inicial,
            viva: true,
        }
    }

    // Método mutável que acrescenta +1 ao corpo da cobra
    pub fn crescer(&mut self){
        self.tamanho += 1;
    }

    // Método imutável. Apenas verifica se a cobra está viva
    pub fn esta_viva(&self) -> bool{
        self.viva
    }
}

fn main(){
    let mut minha_cobra = Cobra::new(3);

    if minha_cobra.esta_viva(){
        minha_cobra.crescer();
        println!("A cobra cresceu. Tamanho: {}", minha_cobra.tamanho);
    }

    minha_cobra.viva = false;
    if minha_cobra.esta_viva() == false{
        println!("A cobra morreu, press f");
    }
}
