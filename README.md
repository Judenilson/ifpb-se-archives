# Sistemas Embarcados - 2022.1

[![NPM](https://img.shields.io/npm/l/react)](https://github.com/Judenilson/projeto_se/blob/main/LICENSE)

## 1. Instituição

-   Instituto Federal da Paraíba - IFPB
-   Campus Campina Grande

## 2. Docente responsável

-   [Professor Dr. Alexandre Sales Vasconcelos](https://github.com/alexandresvifpb)

## 3. Grupo

-   [Antonio Carlos Albuquerque Alves](https://github.com/antonio357)
-   [David Victor Dos Santos Andrade](https://github.com/davidvictor66)
-   [Judenilson Araujo Silva](https://github.com/Judenilson)

## 4. Descrição do projeto

Nesse projeto foi realizado desenvolvimento de um sistema embarcado que é capaz de registrar o controle de acesso de alunos na sala de aula, utilizando tags RFID, além disso, também é possível controlar a abertura de uma porta usando uma trava elétrica e o estado da iluminação da sala através de um relé. A lista de Alunos presentes na sala pode ser vista através de uma página sob o protocolo http e o início e encerramento da aula são enviados para uma conta no Telegram, como forma de aviso.

## 5. Objetivos

### 5.1. Geral

-   O objetivo geral deste trabalho é o desenvolvimento de um hardware de controle de presença de alunos.

### 5.2. Específicos

-   Criar um prototipo que seja capaz de realizar a presença dos alunos, controlando o acesso com trava elétrica na porta e a iluminação do ambiente.

## 6. Resumo das arquiteturas

Todo _Hardware_ apresentado no tópico 6.1 foi construído no aplicativo Fritzzing e o _Firmware e/ou Software/App_ utilizando o Visual Studio Code, com arquitetuda do ESP-IDF.

### 6.1. Desenhos
-   Breadboard

![Visualização do Breadboard](https://github.com/Judenilson/projeto_se/blob/Master/imgs/breadboard.png)

-   PCB

![pcb](https://github.com/Judenilson/projeto_se/blob/Master/imgs/pcb.png)

-   Esquema eletrônico

![Schematic](https://github.com/Judenilson/projeto_se/blob/Master/imgs/circuito_elétrico.png)

## 7. Diagrama de Processo

![Processo](https://github.com/Judenilson/projeto_se/blob/Master/imgs/fluxograma_firmware.jpg)

## 8. Diagrama de Bloco

![Bloco](https://github.com/Judenilson/projeto_se/blob/Master/imgs/diagrama_bloco_hardware.jpg)

## 9. Resumo dos Resultados

Nesse projeto foi possível construir um protótipo capaz de realizar o registro de alunos através de Tags RFIDs, além disso, através do sistema online é possível acender e apagar as luzes da sala de aula, bem como disparar um aviso para toda a turma através do Telegram, informando o início e término da aula. 

## 10. Conclusão

Diante do projeto executado foi possível aprender a instalar e trabalhar com ESP-IDF, bem como, implementar um sistema com RFID, instalar componentes, criar novos componentes e realizar a implementação de threads.
Para desenvolvimento futuro, segundo sugestão do professor Alexandre, fica o desafio de montar um sistema de baixo custo que possibilite realizar o salvamento das listas de presença num cartão de memória, para que o professor tenha possibilidade de escolher se quer manter a lista de presença ou não.
