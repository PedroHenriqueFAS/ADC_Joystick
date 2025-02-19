#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

#include "lib/ssd1306_i2c.h"

// Definição dos pinos para LEDs
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12

// Definição dos pinos dos botões
#define BTN_A 5

// Definição dos pinos do joystick
#define JOY_X 26
#define JOY_Y 27
#define JOY_BTN 22

// Definição dos pinos do display OLED
#define DISPLAY_SDA 14
#define DISPLAY_SCL 15

// Definições relacionadas à resolução do ADC e PWM
#define ADC_WRAP 4096
#define CLK_DIV 16.0

#define ADC_CHANNEL_0 0
#define ADC_CHANNEL_1 1

// Zona morta e valor central para o joystick
#define DEAD_ZONE 200
#define CENTER_VALUE 2048

// Tamanho do display OLED
#define SSD1306_HEIGHT 64
#define SSD1306_WIDTH 128

// Definições da borda do display (sem tocar na borda)
#define BORDER_HEIGHT 63
#define BORDER_WIDTH 127

// Variáveis para gerenciar o buffer do display
uint8_t ssd[ssd1306_buffer_length];

// Estrutura para a área de renderização do display
struct render_area frame_area = {
    start_column : 0,
    end_column : BORDER_WIDTH,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

// Tempo para debouncing
static uint32_t last_time = 0;

// Variáveis de estado para os LEDs
static volatile bool led_r_state = true;
static volatile bool led_g_state = false;
static volatile bool led_b_state = true;

// Flags para controlar a borda do display
static volatile bool display_border_alternate = false;
static volatile bool display_border_on = false;
typedef enum {FIRST_BORDER, SECOND_BORDER, THIRD_BORDER, NO_BORDER} border_type;
static border_type current_border = FIRST_BORDER;

/* 
    Medir a largura e altura do display sem a borda
    largura do display - 8 - 1 (para não tocar na borda) = 119
    altura do display - 8 - 1 (para não tocar na borda) = 55
*/

static uint height_with_square = 55;
static uint width_with_square = 119;

// Prototipação das funções
static void gpio_irq_handler(uint gpio, uint32_t events);
static bool debounce(uint32_t *last_time);
static uint setup_pwm(uint pwm_pin);
static uint16_t read_joystick_axis(uint16_t *vrx, uint16_t *vry);
static void init_hardware();
static uint16_t adjust_value(uint16_t value);
static void draw_square(uint x0, uint y0, bool set);
static void draw_border(bool set);
static void draw_dashed_border(bool set);
static void draw_double_line_border(bool set);
static int map(uint raw_coord_x, uint raw_coord_y);

// Declaração de funções externas do display
extern void calculate_render_area_buffer_length(struct render_area *area);
extern void ssd1306_send_command(uint8_t cmd);
extern void ssd1306_send_command_list(uint8_t *ssd, int number);
extern void ssd1306_send_buffer(uint8_t ssd[], int buffer_length);
extern void ssd1306_init();
extern void ssd1306_scroll(bool set);
extern void render_on_display(uint8_t *ssd, struct render_area *area);
extern void ssd1306_set_pixel(uint8_t *ssd, int x, int y, bool set);
extern void ssd1306_draw_line(uint8_t *ssd, int x_0, int y_0, int x_1, int y_1, bool set);
extern void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character);
extern void ssd1306_draw_string(uint8_t *ssd, int16_t x, int16_t y, char *string);
extern void ssd1306_command(ssd1306_t *ssd, uint8_t command);
extern void ssd1306_config(ssd1306_t *ssd);
extern void ssd1306_init_bm(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c);
extern void ssd1306_send_data(ssd1306_t *ssd);
extern void ssd1306_draw_bitmap(ssd1306_t *ssd, const uint8_t *bitmap);

// Variáveis globais para gerenciar PWM e o joystick
uint slice_r, slice_b;
uint16_t vrx_value, vry_value;
uint x,y;

int main()
{
    init_hardware();

    int prev_x = 0;
    int prev_y = 0;
    bool first = true;

    while (true) {
        
        // Lê os valores convertidos do joystick
        read_joystick_axis(&vrx_value, &vry_value);

        // Mapeia os valores do joystick para a tela
        map(vrx_value, vry_value);

        // Ajusta os valores PWM para os LEDs
        vrx_value = adjust_value(vrx_value);
        vry_value = adjust_value(vry_value);

        // Atualiza a intensidade do LED vermelho
        pwm_set_gpio_level(LED_R_PIN, vrx_value);
        // Atualiza a intensidade do LED azul
        pwm_set_gpio_level(LED_B_PIN, vry_value);

        // Limpa o quadrado anterior
        draw_square(prev_x, prev_y, false);
        
        // Desenha o quadrado no novo local
        draw_square(x,y, true);        

        // Verifica se o botão do joystick foi pressionado
        if(display_border_alternate) {
            switch (current_border) {
            case FIRST_BORDER:
                printf("%s primeira borda\n", display_border_on ? "Desenhando" : "Apagando");
                draw_border(display_border_on);
                break;
            case SECOND_BORDER:
                printf("Desenhando segunda borda\n");
                gpio_put(LED_G_PIN, true);
                draw_dashed_border(display_border_on);
                break;
            case THIRD_BORDER:
                printf("%s terceira borda\n", display_border_on ? "Desenhando" : "Apagando");
                draw_double_line_border(display_border_on);
                break;
            case NO_BORDER:
                printf("Sem borda\n");
                draw_double_line_border(false);
            default:
                break;
            }
            current_border = (current_border + 1) % 4; // Alterna entre as bordas
            display_border_alternate = !display_border_alternate;
        }
        // Salva as coordenadas para limpar o quadrado na próxima iteração
        prev_x = x;
        prev_y = y;
    
        // Renderiza o buffer no display
        render_on_display(ssd, &frame_area);
        sleep_ms(10);
    }
}

void init_hardware() {
    stdio_init_all();

    // Inicializa o pino do botão A
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    // Inicializa o pino do botão do joystick
    gpio_init(JOY_BTN);
    gpio_set_dir(JOY_BTN, GPIO_IN);
    gpio_pull_up(JOY_BTN);

    // Inicializa os LEDs
    slice_r = setup_pwm(LED_R_PIN);
    slice_b = setup_pwm(LED_B_PIN);
    // Inicializa o pino do LED verde
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);

    // Inicializa o joystick
    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);

    // Inicializa o display OLED
    i2c_init(i2c1, 1000000);
    gpio_set_function(DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA);
    gpio_pull_up(DISPLAY_SCL);
    ssd1306_init();

    calculate_render_area_buffer_length(&frame_area);

    // Configura as interrupções para os botões
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(JOY_BTN, GPIO_IRQ_EDGE_FALL, true, gpio_irq_handler);
}

static uint setup_pwm(uint pwm_pin) {
    gpio_set_function(pwm_pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pwm_pin);

    pwm_set_clkdiv(slice, CLK_DIV);
    pwm_set_wrap(slice, ADC_WRAP);
    pwm_set_gpio_level(pwm_pin, 0);
    pwm_set_enabled(slice, true);

    // Frequência do PWM é 1,9 KHz

    return slice;
}

static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (debounce(&last_time)) {
        // Desativa o PWM dos LEDs vermelho e azul
        if(gpio == BTN_A) {
            printf(
                "Botão A pressionado\n"
                "Alternando estado dos PWMs\n"
            );

            led_r_state = !led_r_state;
            led_b_state = !led_b_state;

            pwm_set_enabled(slice_r, led_r_state);
            pwm_set_enabled(slice_b, led_b_state);
            gpio_put(LED_R_PIN, led_r_state);
            gpio_put(LED_B_PIN, led_b_state);
        } else {
            // Muda o estado do LED verde e alterna as bordas
            printf(
                    "Botão joystick pressionado\n"
                    "Alternando estado do LED verde\n"
                    "Alternando borda\n"
                );

            led_g_state = !led_g_state;
            gpio_put(LED_G_PIN, led_g_state);
            
            display_border_alternate = !display_border_alternate;
            display_border_on = !display_border_on;
        }
    }
}

// Função de debouncing
static bool debounce(uint32_t *last_time) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if(current_time - *last_time > 200000) {
        *last_time = current_time;
        return true;
    }
    return false;
}

// Função para ajustar os valores do joystick
static uint16_t read_joystick_axis(uint16_t *vrx, uint16_t *vry) {
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vry = adc_read();

    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    *vrx = adc_read();
}

// Função para ajustar a zona morta do joystick e controlar a intensidade dos LEDs
static uint16_t adjust_value(uint16_t value) {
    if (value > CENTER_VALUE + DEAD_ZONE) {
        return value - (CENTER_VALUE + DEAD_ZONE);
    } else if (value < CENTER_VALUE - DEAD_ZONE) {
        return (CENTER_VALUE - DEAD_ZONE) - value;
    } else {
        return 0;
    }
}

static void draw_square(uint x0, uint y0, bool set) {
    ssd1306_draw_line(ssd, x0, y0, x0 + 8, y0, set);
    ssd1306_draw_line(ssd, x0, y0, x0, y0 + 8, set);
    ssd1306_draw_line(ssd, x0, y0 + 8, x0 + 8, y0 + 8, set);
    ssd1306_draw_line(ssd, x0 + 8, y0, x0 + 8, y0 + 8, set);
}

// Funções para desenhar bordas

// Borda 1
static void draw_border(bool set) {
    ssd1306_draw_line(ssd, 0, 0, BORDER_WIDTH, 0, set);
    ssd1306_draw_line(ssd, 0, 0, 0, BORDER_HEIGHT, set);
    ssd1306_draw_line(ssd, BORDER_WIDTH, 0, BORDER_WIDTH, BORDER_HEIGHT, set);
    ssd1306_draw_line(ssd, 0, BORDER_HEIGHT, BORDER_WIDTH, BORDER_HEIGHT, set);
}

// Borda 2
static void draw_dashed_border(bool set) {
    for (int i = 0; i < BORDER_WIDTH; i += 4) {
        ssd1306_set_pixel(ssd, i, 0, set);
        ssd1306_set_pixel(ssd, i, BORDER_HEIGHT, set);
    }
    for (int i = 0; i < BORDER_HEIGHT; i += 4) {
        ssd1306_set_pixel(ssd, 0, i, set);
        ssd1306_set_pixel(ssd, BORDER_WIDTH, i, set);
    }
}

// Borda 3
static void draw_double_line_border(bool set) {
    // Borda externa
    ssd1306_draw_line(ssd, 0, 0, BORDER_WIDTH, 0, set);
    ssd1306_draw_line(ssd, 0, 0, 0, BORDER_HEIGHT, set);
    ssd1306_draw_line(ssd, BORDER_WIDTH, 0, BORDER_WIDTH, BORDER_HEIGHT, set);
    ssd1306_draw_line(ssd, 0, BORDER_HEIGHT, BORDER_WIDTH, BORDER_HEIGHT, set);

    // Borda interna
    ssd1306_draw_line(ssd, 3, 3, BORDER_WIDTH - 3, 3, set);
    ssd1306_draw_line(ssd, 3, 3, 3, BORDER_HEIGHT - 3, set);
    ssd1306_draw_line(ssd, BORDER_WIDTH - 3, 3, BORDER_WIDTH - 3, BORDER_HEIGHT - 3, set);
    ssd1306_draw_line(ssd, 3, BORDER_HEIGHT - 3, BORDER_WIDTH - 3, BORDER_HEIGHT - 3, set);
}

static int map(uint raw_coord_x, uint raw_coord_y) {
    uint min_x, min_y;
    uint max_x, max_y; 

    // Garante que o quadrado não toque nas bordas
    if (display_border_on || FIRST_BORDER + 1) {
        min_x = 1;
        min_y = 1;
        max_x = width_with_square - 1;
        max_y = height_with_square - 1;
    }
    if (current_border == THIRD_BORDER + 1) {
        min_x = 4;
        min_y = 4;
        max_x = width_with_square - 4;
        max_y = height_with_square - 4;
    }
    
    x = (raw_coord_x * max_x) / 4084;
    y = max_y - (raw_coord_y * max_y) / 4084; // Invertendo o eixo Y

    // Garantir que x e y não ultrapassem os limites
    if (x < min_x) x = min_x;
    if (y < min_y) y = min_y;
    if (x > max_x) x = max_x;
    if (y > max_y) x = max_y;
}
