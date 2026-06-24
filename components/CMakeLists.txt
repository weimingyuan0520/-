set(src_dirs
            LED
            BEEP
            RELAY
            KEY
            SENSOE-DO
            dc_motor
            step_motor
            exit
            GPTIM
            LCD
            SPI
            PWM
            MOTOR            
            continuousADC
            single-shotADC
            UART
          )


set(include_dirs
            LED
            BEEP
            RELAY
            KEY
            SENSOE-DO
            dc_motor
            step_motor
            exit
            GPTIM
            LCD
            SPI
            PWM
            MOTOR            
            continuousADC
            single-shotADC
            UART
          )
            

set(requires
            driver
            freertos
            esp_adc)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)