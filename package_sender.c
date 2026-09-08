#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    // 1. Configurações do destino
    char *ip_destino = "10.0.0.1";
    int porta_destino = 8081;
    char *mensagem = "A Exploração Espacial e a Busca pelo Nosso Lugar no Universo A curiosidade é uma das forças mais fundamentais e transformadoras da natureza humana. Desde os primórdios da civilização, quando nossos ancestrais olhavam para o céu noturno e tentavam decifrar o movimento dos pontos luminosos no firmeza, a humanidade busca compreender o cosmos e o seu próprio lugar nele. O que começou como mitologia e astronomia observacional evoluiu, ao longo dos séculos, para uma das jornadas mais ambiciosas da ciência moderna: a exploração espacial. A era espacial propriamente dita teve início em meados do século XX, impulsionada pela rivalidade geopolítica da Guerra Fria. O lançamento do satélite Sputnik 1 pela União Soviética em 1957 e, posteriormente, a chegada do ser humano à Lua com a missão Apollo 11 dos Estados Unidos em 1969, marcaram momentos decisivos na história da tecnologia. Pela primeira vez, a vida terrestre deixou o seu berço planetário e estendeu sua presença para além da atmosfera. O que começou como uma demonstração de poder político e militar rapidamente se converteu em uma busca científica global de proporções incalculáveis. Nas décadas seguintes, o foco da exploração espacial expandiu-se drasticamente. O envio de sondas automáticas para os confins do Sistema Solar permitiu catalogar e fotografar mundos distantes de forma sem precedentes. Naves como as missões Voyager revelaram a complexidade dos planetas gasosos gigantes, seus anéis e suas luas fascinantes, muitas das quais possuem oceanos subterrâneos com potencial para abrigar vida. Paralelamente, os robôs enviados a Marte, como o Curiosity e o Perseverance, continuam a analisar o solo e a atmosfera do planeta vermelho em busca de vestígios de água líquida e assinaturas químicas de vida microbiológica passada. Além da investigação de nossos vizinhos planetários, telescópios espaciais como o Hubble e, mais recentemente, o James Webb, revolucionaram a astrofísica e a cosmologia. Ao operarem fora das distorções provocadas pela atmosfera terrestre, esses instrumentos conseguem captar a luz emitida por galáxias primordiais formadas há mais de 13 bilhões de anos, logo após o Big Bang. Essas observações permitem mapear a expansão do universo, investigar a natureza da matéria escura e da energia escura e identificar milhares de exoplanetas — planetas que orbitam estrelas fora do nosso Sistema Solar —, alguns dos quais possuem condições potencialmente habitáveis. No entanto, o impacto da exploração espacial vai muito além das descobertas científicas abstratas. As tecnologias desenvolvidas para superar os desafios do espaço geraram inúmeros benefícios práticos para o cotidiano na Terra. A rede de satélites que orbita o planeta hoje é essencial para as comunicações globais, a navegação por GPS, a previsão meteorológica e o monitoramento das mudanças climáticas e do desmatamento. Materiais avançados, equipamentos médicos de precisão, sistemas de filtragem de água e até mesmo a tecnologia das câmeras dos smartphones modernos têm raízes diretas na pesquisa e no desenvolvimento aeroespacial. Atualmente, vivemos um novo capítulo dessa jornada, caracterizado pela democratização do acesso ao espaço e pela forte presença do setor privado. A redução drástica nos custos de lançamento, impulsionada pelo desenvolvimento de foguetes reutilizáveis, abriu portas para uma economia orbital em rápida expansão. Projetos de retorno sustentável à Lua, a construção de novas estações espaciais comerciais e os planos para a futura colonização de Marte deixaram de ser mera ficção científica e tornaram-se alvos reais de investimentos e planejamentos governamentais e empresariais. A longo prazo, a exploração espacial não representa apenas um impulso pelo conhecimento ou pelo avanço econômico, mas também uma garantia de sobrevivência para a espécie humana. Tornar-nos uma civilização multiplanetária é a forma mais eficaz de proteger a humanidade contra eventos de extinção em massa, sejam eles provocados por desastres naturais, como o impacto de um grande asteroide, ou por crises ambientais e tecnológicas de nossa própria autoria. Em última análise, olhar para o espaço é olhar para o nosso próprio reflexo. Ao contemplar a imensidão do cosmos e a fragilidade do nosso pálido ponto azul, como bem descreveu o astrônomo Carl Sagan, somos lembrados da responsabilidade coletiva de preservar o nosso planeta natal, enquanto continuamos a expandir os horizontes da consciência humana pelo universo. World";
    
    int sock;
    struct sockaddr_in destino;

    // 2. Criação do Socket
    // AF_INET = Família IPv4 (Garante que o primeiro byte será 0x45)
    // SOCK_DGRAM = Datagrama UDP (Garante que o campo protocolo será 17)
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar o socket");
        return 1;
    }

    // 3. Zera a estrutura e configura os dados do destino
    memset(&destino, 0, sizeof(destino));
    destino.sin_family = AF_INET;
    
    // htons = Host TO Network Short (Lembra da inversão do Endianness? 
    // Isso garante que a porta seja enviada na ordem certa para a rede)
    destino.sin_port = htons(porta_destino);
    
    // Converte a string do IP ("10.0.0.5") para o formato binário de 32 bits
    if (inet_pton(AF_INET, ip_destino, &destino.sin_addr) <= 0) {
        perror("Erro ao converter o endereço IP");
        close(sock);
        return 1;
    }

    printf("Enviando mensagem para %s:%d...\n", ip_destino, porta_destino);

    // 4. Dispara o pacote para o Kernel do Linux
    int bytes_enviados = sendto(sock, mensagem, strlen(mensagem), 0,
                                (struct sockaddr *)&destino, sizeof(destino));

    if (bytes_enviados < 0) {
        perror("Erro ao enviar o pacote");
    } else {
        printf("Sucesso! %d bytes de dados entregues ao Kernel.\n", bytes_enviados);
    }

    // 5. Fecha o socket
    close(sock);
    return 0;
}
