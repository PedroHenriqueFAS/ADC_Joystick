# Projeto: Controle de LEDs RGB e Display SSD1306 com Joystick

## 📝 Sobre o Projeto

Este projeto foi desenvolvido para controlar LEDs RGB e um display OLED SSD1306 utilizando um microcontrolador Raspberry Pi Pico W. O controle dos LEDs e o movimento do quadrado no display são feitos por meio de um joystick analógico.

## 🎯 Funcionalidades

1. Controle dos LEDs: O projeto utiliza um joystick analógico para ajustar a intensidade dos LEDs vermelhos e azuis. O LED verde é controlado por um botão.
2. Movimento do quadrado: O movimento de um quadrado na tela é controlado pelos eixos X e Y do joystick, com mapeamento de valores para garantir que o quadrado não toque nas bordas do display.
3. Bordas alternáveis: O código oferece a capacidade de desenhar três tipos de bordas no display:
    - Borda simples
    - Borda tracejada
    - Borda dupla
    O tipo de borda é alternado a cada pressão de um botão.
4. Debounce de botões: O código implementa uma função de debounce para garantir que os botões não sejam acionados acidentalmente devido a ruídos elétricos.

## 🔧 Configuração de Pinos

- LEDs RGB: Utilizam os pinos GPIO 13 (vermelho), 11 (verde) e 12 (azul).
- Joystick:
    - Eixo X: GPIO 26
    - Eixo Y: GPIO 27
    - Botão do joystick: GPIO 22
- Display SSD1306:
    - SDA: GPIO 14
    - SCL: GPIO 15
- Botões de controle: GPIO 5 para um botão adicional.

## 🔓 Como Usar

1. Iniciar o sistema: Quando o código é iniciado, o quadrado será desenhado no display e seu movimento será controlado pelo joystick. A intensidade dos LEDs vermelhos e azuis será controlada conforme o movimento do joystick.
2. Alterar bordas: Pressione o botão do joystick para alternar entre os tipos de borda e alternar o estado do LED verde.
3. Acionar LEDs: Pressione o botão BTN_A para alternar o estado dos LEDs vermelhos e azuis, além de ativar/desativar a PWM nos LEDs.

## 📓 Bibliotecas e Dependências

- Pico SDK: Biblioteca oficial para o Raspberry Pi Pico.
- SSD1306 I2C: Biblioteca para controle do display OLED.

## 🛠️ Requisitos

- Raspberry Pi Pico W
- LEDs RGB (Vermelho, Verde e Azul)
- Joystick analógico
- Display SSD1306 I2C
- Botões de controle

## ⚙️ Compilação e Execução

1. Clone o repositório do projeto:
   ```sh
   git clone https://github.com/PedroHenriqueFAS/ADC_Joystick
   cd ADC_Joystick
   ```
2. Crie um diretório de build e entre nele:
   ```sh
   mkdir build
   cd build
   ```
3. Execute o comando CMake para configurar a compilação:
   ```sh
   cmake ..
   ```
4. Compile o projeto:
   ```sh
   make
   ```
5. Faça o upload do binário gerado para a Raspberry Pi Pico.

## 👥 Colaboradores

1. **Pedro Henrique Ferreira Amorim da Silva** - [GitHub](https://github.com/PedroHenriqueFAS)

## 📜 Licença

Este projeto está licenciado sob a Licença MIT. Para mais detalhes, consulte o arquivo LICENSE.

