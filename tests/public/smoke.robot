*** Settings ***
Documentation    Public smoke tests for firmware challenge
Suite Setup      Setup Suite
Suite Teardown   Teardown Suite
Test Setup       Reset Firmware State
Library          RobotFrameworkRenode

*** Variables ***
${UART1_TIMEOUT}    2s
${SIMULATION_FILE}  ${CURDIR}/../../platform/run.resc

*** Keywords ***
Setup Suite
    [Documentation]    Initialize Renode and start simulation
    Setup
    Execute Command    mach create "stm32f411"
    Execute Command    machine LoadPlatformDescription @${CURDIR}/../../platform/stm32f411.repl
    Execute Command    sysbus LoadELF @${CURDIR}/../../firmware/build/firmware.elf
    Execute Command    emulation CreateServerSocketTerminal 5555 "diag_shell" false
    Execute Command    connector Connect sysbus.uart1 diag_shell
    Create Terminal Tester    sysbus.uart1    timeout=2
    Start Emulation

Teardown Suite
    [Documentation]    Stop simulation and cleanup
    Teardown

Reset Firmware State
    [Documentation]    Reset firmware state between tests
    Execute Command    sysbus.uart1 WriteLine "RESET"
    Wait For Line On Uart    OK    timeout=1

Send Packet To UART2
    [Documentation]    Send a binary packet to UART2
    [Arguments]    @{bytes}
    FOR    ${byte}    IN    @{bytes}
        Execute Command    sysbus.uart2 WriteChar ${byte}
    END

Compute CRC8
    [Documentation]    Compute CRC8 for given bytes using helper script
    [Arguments]    @{bytes}
    ${result}=    Run Process    python3    ${CURDIR}/../../scripts/crc8.py    @{bytes}
    ${crc}=    Strip String    ${result.stdout}
    [Return]    ${crc}

*** Test Cases ***
T01 - PA5 Toggles After Boot
    [Documentation]    Verify watchdog LED toggles every 500ms
    [Tags]    smoke    gpio    watchdog

    # Read initial PA5 state
    Execute Command    emulation RunFor "0.6"
    ${state1}=    Execute Command    sysbus.gpioPortA 5 State
    Should Not Be Empty    ${state1}

    # Wait another 600ms and check state changed
    Execute Command    emulation RunFor "0.6"
    ${state2}=    Execute Command    sysbus.gpioPortA 5 State
    Should Not Be Empty    ${state2}

    # States must differ (toggled)
    Should Not Be Equal    ${state1}    ${state2}

T02 - Valid Packet Is Stored
    [Documentation]    Verify valid packet is stored and appears in DUMP
    [Tags]    smoke    protocol    storage

    # Build packet: 0xA5 0x5A [sensor_id] [len] [payload...] [CRC]
    ${crc}=    Compute CRC8    0xA5    0x5A    0x01    0x02    0xAB    0xCD
    Send Packet To UART2    0xA5    0x5A    0x01    0x02    0xAB    0xCD    ${crc}

    # Give time to process
    Execute Command    emulation RunFor "0.05"

    # Request DUMP
    Execute Command    sysbus.uart1 WriteLine "DUMP"
    Wait For Line On Uart    S1    timeout=1
    ${line}=    Wait For Line On Uart    last=ABCD    timeout=1
    Should Contain    ${line}    ABCD

T03 - STATUS Command Responds
    [Documentation]    Verify STATUS command returns sensor count
    [Tags]    smoke    command

    # Send valid packets for sensors 0x00 and 0x01
    ${crc0}=    Compute CRC8    0xA5    0x5A    0x00    0x01    0xAA
    Send Packet To UART2    0xA5    0x5A    0x00    0x01    0xAA    ${crc0}

    ${crc1}=    Compute CRC8    0xA5    0x5A    0x01    0x01    0xBB
    Send Packet To UART2    0xA5    0x5A    0x01    0x01    0xBB    ${crc1}

    Execute Command    emulation RunFor "0.1"

    # Request STATUS
    Execute Command    sysbus.uart1 WriteLine "STATUS"
    ${response}=    Wait For Line On Uart    sensors=    timeout=1
    Should Contain    ${response}    sensors=2

T04 - RESET Clears State
    [Documentation]    Verify RESET command clears all sensor data
    [Tags]    smoke    command

    # Send a valid packet
    ${crc}=    Compute CRC8    0xA5    0x5A    0x00    0x01    0xFF
    Send Packet To UART2    0xA5    0x5A    0x00    0x01    0xFF    ${crc}

    Execute Command    emulation RunFor "0.05"

    # Reset
    Execute Command    sysbus.uart1 WriteLine "RESET"
    Wait For Line On Uart    OK    timeout=1

    # DUMP should show no sensors
    Execute Command    sysbus.uart1 WriteLine "DUMP"
    ${response}=    Wait For Line On Uart    END    timeout=1
    Should Not Contain    ${response}    S0

T05 - Unknown Command Returns ERR
    [Documentation]    Verify unknown commands return error
    [Tags]    smoke    command

    Execute Command    sysbus.uart1 WriteLine "FOOBAR"
    ${response}=    Wait For Line On Uart    ERR    timeout=1
    Should Contain    ${response}    ERR

T06 - Help Command Lists Available Commands
    [Documentation]    Verify ? command lists commands
    [Tags]    smoke    command

    Execute Command    sysbus.uart1 WriteLine "?"
    Wait For Line On Uart    STATUS    timeout=1
    Wait For Line On Uart    END    timeout=1
