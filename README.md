# PATOS/POMBO PSEL Open Source / Comunidade

## Sobre mim

Sou o Vitor Lima dos Santos, 025 de ciência da computação na UFSCar. Nessa candidatura, apresento a contribuição que realizei durante o **Google Summer of Code 2026**, um programa de contribuição a projetos Open Source do Google.

## Projeto Open Source

**Projeto:** OSIPI ASL Methods Section Generator, especificamente o pacote Python `pyaslreport`, que é o núcleo da ferramenta.
**Linguagens:** Python (toda a minha contribuição). O repositório completo também usa Python/FastAPI (backend) e TypeScript/Next.js (frontend), mas meu trabalho foi inteiramente em Python.
**Repositório:** https://github.com/OSIPI/Method-section-generator
**Pull Request aceito (mergeado):** https://github.com/OSIPI/Method-section-generator/pull/66
**Pull Request de continuação:** https://github.com/OSIPI/Method-section-generator/pull/67 

## Contexto do projeto

A **OSIPI** (Open Science Initiative for Perfusion Imaging) é uma iniciativa científica ligada à International Society for Magnetic Resonance in Medicine voltada à padronização de imagens de perfusão por ressonância magnética. O projeto em que trabalhei, o *ASL Methods Section Generator*, é uma ferramenta que lê metadados de aquisições de **Arterial Spin Labeling (ASL)** (uma técnica de ressonância magnética que mede fluxo sanguíneo cerebral) a partir de datasets nos formatos **BIDS** e **DICOM**, valida os parâmetros de aquisição e gera automaticamente o texto padronizado da seção de *Methods* que pesquisadores incluem em artigos científicos.

## Como conheci o projeto

Cheguei ao projeto por meio do **Google Summer of Code 2026**. Desde o ano passado conheço o programa, e nesse ano decidi aplicar. Durante o período de aplicação, pesquisei projetos e instituições em potencial com temas que se alinhassem com meu conhecimento e meu interesse. Lembro de ter filtrado somente os projetos que envolviam python e, a partir deles, ver alguma área que pudesse agregar mais.

## Como surgiu minha contribuição

Quando comecei, o pacote da ferramenta **não tinha nenhum teste** para sua lógica de processamento e **nenhum pipeline de CI** rodando a cada mudança. Entretanto, eles tinham a intenção de expandir a ferramenta, adicionando novas funções. Com isso, surgiu a preocupação de garantir que durante esse processo não houvesse nenhuma regressão no funcionamento atual. 

Minha proposta original, *"Strengthening `pyaslreport` reliability through automated testing and CI"*,  foi construir um conjunto de testes em torno da arquitetura atual do pacote e integrá-la a um workflow de CI. O objetivo disso tudo, na época, era tratado de maneira bem vaga. Por isso, entrei em contato com os responsáveis pelo potencial projeto mostrando o draft da minha proposta para garantir que estava alinhado com as expectativas deles. Após alguns dias, recebi um feedback de um deles que, dentre outras coisas, mencionava que queriam um runner que funcionasse localmente a partir de qualquer conjunto de exemplos e, no CI, a partir um conjunto menor commitado. Felizmente, tive tempo o suficiente para adaptar minha proposta e garantir que estava próxima ao que eles queriam. 


## Minha contribuição

A contribuição tem duas partes complementares.

**1. Suíte de testes automatizados + CI.**  
Estruturei uma suíte de testes automatizados e reutilizáveis com `pytest` para garantir que as principais etapas do projeto funcionassem corretamente. Os testes verificam o processamento de arquivos de imagem médica e seus metadados, incluindo padronização de informações, conversão de unidades, validação de dados e tratamento de diferentes formatos de entrada, como BIDS e DICOM. Também foram incluídos testes para situações menos comuns e possíveis erros durante a geração dos relatórios.

Além disso, configurei um workflow no **GitHub Actions** para executar esses testes automaticamente em Python 3.11 e 3.12 a cada alteração no código, separando testes unitários e de integração e verificando também a formatação do projeto.

Em termos mais específicos, a suíte inclui fixtures compartilhadas para arquivos NIfTI, metadados de ASL, processadores e contextos de processamento. Os testes cobrem a normalização de metadados e conversão de unidades de tempo, as seis classes de validadores baseados em schema, a consistência e o timing dos dados de M0, a validação de arquivos `aslcontext.tsv`, o agrupamento de arquivos BIDS e DICOM e diferentes casos de borda relacionados à geração dos relatórios.

**2. Confiabilidade de DICOM, robustez de relatório e exemplos de integração.**
Ao rodar os novos testes contra dados reais de fornecedores (GE e Siemens), eles expuseram vários problemas existentes no pacote, que corrigi e cobri com testes de regressão. Alguns exemplos: 
- **Reparo de anonimização GE:** alguns DICOMs anonimizados continham valores ASL importantes corrompidos como bytes de ponto flutuante; adicionei um reparo conservador em memória para um allowlist pequeno de tags, sem modificar os arquivos originais.
- **Seleção determinística de header representativo:** em vez de depender do primeiro arquivo lido, o pacote passa a escolher o header com metadados ASL mais completos.
- **Bug de unidade de tempo GE:** valores de timing GE ficam em milissegundos, mas o contrato interno esperava segundos, o que podia multiplicar por 1000 duas vezes. Normalizei a extração para segundos, com teste cobrindo o ciclo completo `ms → s → ms`.
- **Contrato de retorno entre fornecedores:** normalizei `get_bids_metadata()` para retornar sempre `(metadata, asl_context)`, corrigindo uma falha ponta a ponta no caminho Siemens.
- **Overlay de JSON suplementar, extração de `AcquisitionVoxelSize`, refatoração do parágrafo de Methods em cláusulas condicionais**, entre outras melhorias.

Também completei a camada de integração com **seis exemplos reais commitados** (quatro BIDS e dois DICOM GE), de modo que o job de integração passou a exercitar datasets reais no CI.

## Implementação

O ponto central da arquitetura é um **runner de testes baseado em exemplos**: um novo caso de integração é essencialmente uma pasta de arquivos de entrada mais um `expected_output.json`. O runner descobre os exemplos automaticamente, reconstrói as entradas, executa o caminho real de geração de relatório e compara o resultado completo, **incluindo o texto gerado**, com o resultado esperado commitado. Adicionar um caso novo não exige escrever código, apenas colocar uma pasta.

Decisões técnicas relevantes: comparação do contrato completo de saída (não só campos estruturados), flag `--update-expected` para regenerar resultados quando uma mudança de comportamento é intencional, pareamento determinístico de ASL/M0. 

## Desafios

Foram muitos... Querendo ou não, eu nunca tinha feito algo parecido. Dentre elas:

- **Entender uma codebase desconhecida** antes de escrever qualquer linha de código, e na verdade antes mesmo de escrever a minha proposta, foi necessário um vasto estudo não só da codebase mas também dos termos e conceitos de domínio específicos da área. 
- **Me familizarizar com os termos de domínio específicos:** tanto os códigos quanto as interações com os mentores contavam com diversos termos específicos do âmbito de imagens médicas e ASL no geral. Meu conhecimento prévio não cobria grande parte desses termos, por isso foi necessária uma pesquisa independente. 
- **Trabalhar em volta de código quebrado que não era meu:** mesmo estando fora do escopo do projeto, entregar algo que pudesse ser validado implicava consertar problemas específicos em partes de terceiros, o que gerava insegurança, principalmente pelo fato de eu não estar inserido no domínio médico e não querer deixar passar algo por essa falta de conhecimento.

## Experiência geral

Foi minha primeira experiência contribuindo de forma sustentada para um projeto Open Source real, mantido por outras pessoas, com revisão de código e colaboração assíncrona entre fusos horários diferentes. Aprendi bastante sobre o fluxo de trabalho de OSS: forks, PRs, revisão, CI e sobre como comunicar decisões técnicas de forma que um revisor consiga entender e confiar na mudança. Trabalhar com os mentores tornou a experiência produtiva e agradável, e me deixou confortável para entrar e contribuir em bases de código que eu não escrevi.


### Registro complementar

Escrevi também um relatório final do projeto (formato blog), com mais contexto:
- https://vtorlima.hashnode.dev/google-summer-of-code-2026-with-osipi-strengthening-pyaslreport-reliability