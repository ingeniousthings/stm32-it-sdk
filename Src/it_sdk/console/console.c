/* ==========================================================
 * console.c -  debug & config console
 * Project : Disk91 SDK
 * ----------------------------------------------------------
 * Created on: 9 f�vr. 2019
 *     Author: Paul Pinault aka Disk91
 * ----------------------------------------------------------
 * Copyright (C) 2019 Disk91
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU LESSER General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------
 * 
 *
 * ==========================================================
 */
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>


#include <it_sdk/config.h>
#if ITSDK_WITH_CONSOLE == __ENABLE
#include <it_sdk/console/console.h>
#include <it_sdk/logger/logger.h>
#include <it_sdk/wrappers.h>
#include <it_sdk/time/time.h>
#include <it_sdk/lowpower/lowpower.h>
#if ITSDK_WITH_SECURESTORE == __ENABLE
#include <it_sdk/eeprom/securestore.h>
#endif

static itsdk_console_state_t __console;
static itsdk_console_chain_t __console_head_chain;

static void _itsdk_console_processLine();
static void _itsdk_console_processChar(char c);

/**
 * Dafault Operation
 */

static itsdk_console_return_e _itsdk_console_private(char * buffer, uint8_t sz) {

	if ( sz == 1 ) {
		switch ( buffer[0] ) {
		case '?':
			// help
			_itsdk_console_printf("X          : exit console\r\n");
			_itsdk_console_printf("R          : reset device\r\n");
			_itsdk_console_printf("l / L      : switch LowPower ON / OFF\r\n");
			_itsdk_console_printf("c          : print device config\r\n");
			_itsdk_console_printf("s          : print device state\r\n");
			_itsdk_console_printf("t          : print current time in S\r\n");
			_itsdk_console_printf("T          : print current temperature in oC\r\n");
			_itsdk_console_printf("b          : print battery level\r\n");
			_itsdk_console_printf("B          : print VCC level\r\n");
			_itsdk_console_printf("r          : print last Reset Cause\r\n");

			return ITSDK_CONSOLE_SUCCES;
		case 'X':
			// exit console
			__console.loginState=0;
			_itsdk_console_printf("OK\r\n");
			return ITSDK_CONSOLE_SUCCES;
		case 't':
			// print time
			_itsdk_console_printf("Run time is %d s\r\n",(uint32_t)(itsdk_time_get_ms()/1000L));
			_itsdk_console_printf("OK\r\n");
			return ITSDK_CONSOLE_SUCCES;
		case 'T':
			// print temperature
			{
			uint16_t t = adc_getTemperature();
			_itsdk_console_printf("Temperature is %d.%doC\r\n",t/100,t-((t/100)*100));
			_itsdk_console_printf("OK\r\n");
			return ITSDK_CONSOLE_SUCCES;
			}
		case 'b':
			// battery level
			_itsdk_console_printf("Battery level %dmV\r\n",(uint32_t)(adc_getVBat()));
			_itsdk_console_printf("OK\r\n");
			return ITSDK_CONSOLE_SUCCES;
		case 'B':
			// Vcc level
			_itsdk_console_printf("VCC level %dmV\r\n",(uint32_t)(adc_getVdd()));
			_itsdk_console_printf("OK\r\n");
			return ITSDK_CONSOLE_SUCCES;
		case 'r':
			// Last Reset cause
			_itsdk_console_printf("Reset: ");
			switch(itsdk_getResetCause()) {
			case RESET_CAUSE_BOR: _itsdk_console_printf("BOR\r\n"); break;
			case RESET_CAUSE_RESET_PIN: _itsdk_console_printf("RESET PIN\r\n"); break;
			case RESET_CAUSE_POWER_ON: _itsdk_console_printf("POWER ON\r\n"); break;
			case RESET_CAUSE_SOFTWARE: _itsdk_console_printf("SOFT\r\n"); break;
			case RESET_CAUSE_IWDG: _itsdk_console_printf("IWDG\r\n"); break;
			case RESET_CAUSE_WWDG: _itsdk_console_printf("WWDG\r\n"); break;
			case RESET_CAUSE_LOWPOWER: _itsdk_console_printf("LOW POWER"); break;
			default:
				_itsdk_console_printf("UNKNOWN\r\n"); break;
			}
			_itsdk_console_printf("OK\r\n");
			return ITSDK_CONSOLE_SUCCES;
		case 'R':
			// Reset device
			_itsdk_console_printf("OK\r\n");
			itsdk_reset();
			_itsdk_console_printf("KO\r\n");			// never reached...
			return ITSDK_CONSOLE_FAILED;
		case 'l':
			// switch lowPower On
			lowPower_enable();
			_itsdk_console_printf("OK\r\n");
			return ITSDK_CONSOLE_SUCCES;
		case 'L':
			// switch LowPower Off
			lowPower_disable();
			_itsdk_console_printf("OK\r\n");
			return ITSDK_CONSOLE_SUCCES;
		}
	}
	return ITSDK_CONSOLE_NOTFOUND;

}
static itsdk_console_return_e _itsdk_console_public(char * buffer, uint8_t sz) {

	if ( sz == 1 ) {
		switch ( buffer[0] ) {
		case '?':
			// help
			_itsdk_console_printf("--- Common\r\n");
			_itsdk_console_printf("?          : print help\r\n");
			_itsdk_console_printf("!          : print copyright\r\n");
			_itsdk_console_printf("v          : print version\r\n");
			return ITSDK_CONSOLE_SUCCES;
			break;
		case '!':
			// Copyright
			_itsdk_console_printf("IT_SDK - (c) 2019 - Paul Pinault aka Disk91\r\n");
			_itsdk_console_printf(ITSKD_CONSOLE_COPYRIGHT);
			return ITSDK_CONSOLE_SUCCES;
			break;
		case 'v':
			// Version
			_itsdk_console_printf("FW Version %s\r\n",ITSDK_USER_VERSION);
			_itsdk_console_printf("Build %s %s\r\n",__DATE__, __TIME__);
			_itsdk_console_printf("IT_SDK Version %s\r\n",ITSDK_VERSION);
			return ITSDK_CONSOLE_SUCCES;
			break;
		}
	}
	return ITSDK_CONSOLE_NOTFOUND;
}



/**
 * Setup the console & associated chain
 */
void itsdk_console_setup() {
	__console.expire = 0;
	__console.loginState = 0;
	__console.pBuffer = 0;
	__console_head_chain.console_private = _itsdk_console_private;
	__console_head_chain.console_public = _itsdk_console_public;
	__console_head_chain.next = NULL;
}


/**
 * This function is call on every wake-up to proceed the pending characters on the serial
 * port and call the associated services.
 */
void itsdk_console_loop() {

	char c;
	serial_read_response_e r;

	// Check the expiration
	if ( __console.loginState == 1 ) {
		uint64_t s = itsdk_time_get_ms()/1000;
		if ( __console.expire < s ) {
			 __console.loginState = 0;
			 _itsdk_console_printf("logout\r\n");
		}
	}

  #if ( ITSDK_CONSOLE_SERIAL & __UART_LPUART1 ) > 0
	do {
		 r = serial1_read(&c);
		 if ( r == SERIAL_READ_SUCCESS || r == SERIAL_READ_PENDING_CHAR) {
			 _itsdk_console_processChar(c);
		 }
	} while ( r == SERIAL_READ_PENDING_CHAR );
  #endif
  #if ( ITSDK_CONSOLE_SERIAL & __UART_USART2 ) > 0
	do {
		 r = serial2_read(&c);
		 if ( r == SERIAL_READ_SUCCESS || r == SERIAL_READ_PENDING_CHAR) {
			 _itsdk_console_processChar(c);
		 }
	} while ( r == SERIAL_READ_PENDING_CHAR );
  #endif

}

// =================================================================================================
// Processing output
// =================================================================================================

void _itsdk_console_printf(char *format, ...) {
	va_list args;
	char 	fmtBuffer[LOGGER_MAX_BUF_SZ]; 				// buffer for log line formating before printing
    va_start(args,format);
	vsnprintf(fmtBuffer,LOGGER_MAX_BUF_SZ,format,args);
	va_end(args);
#if ( ITSDK_CONSOLE_SERIAL & __UART_LPUART1 ) > 0
	serial1_print(fmtBuffer);
#endif
#if ( ITSDK_CONSOLE_SERIAL & __UART_USART2 ) > 0
	serial2_print(fmtBuffer);
#endif
}

// =================================================================================================
// Processing input
// =================================================================================================

static void _itsdk_console_processLine() {

	// Empty line
	if ( __console.pBuffer == 0 ) return;

	// Clean the buffer
	if ( __console.pBuffer > 0 && __console.serialBuffer[__console.pBuffer-1] == '\r' ) {
		__console.pBuffer--;
	}
	for ( int i = __console.pBuffer ; i < ITSDK_CONSOLE_LINEBUFFER ; i++) {
		__console.serialBuffer[i] = 0;
	}

	if ( __console.loginState == 0 ) {
		// console locked

		// We are going to remove the possible \r and create a 16B array with leading 0 to match with
		// the console password field in Secure Store
		// Password max size is 15 byte.
		if ( __console.pBuffer < 16 ) {
			 __console.loginState=1;
			#if ITSDK_WITH_SECURESTORE == __DISABLE
				uint8_t passwd[16] = ITSDK_SECSTORE_CONSOLEKEY;
			#else
				uint8_t passwd[16];
				itsdk_secstore_readBlock(ITSDK_SS_CONSOLEKEY, passwd);
			#endif
				for ( int i = 0 ; i < 16 ; i++) {
					if (__console.serialBuffer[i] != passwd[i] && __console.loginState == 1) __console.loginState=0;
				}
				bzero(passwd,16);
		}
		if ( __console.loginState == 1 ) {
			// Login sucess
			uint64_t s = itsdk_time_get_ms()/1000;
			__console.expire = (uint32_t)s + ITSDK_CONSOLE_EXPIRE_S;
			_itsdk_console_printf("OK\r\n");
		} else {
			// Login Failed This can be a public operation request
			itsdk_console_chain_t * c = &__console_head_chain;
			itsdk_console_return_e  ret = ITSDK_CONSOLE_NOTFOUND;
			while ( c != NULL ) {
			   switch ( c->console_public((char*)__console.serialBuffer,(uint8_t)__console.pBuffer) ) {
				   case ITSDK_CONSOLE_SUCCES:
				   case ITSDK_CONSOLE_FAILED:
					   ret = ITSDK_CONSOLE_SUCCES;
					   break;
				   default:
					   break;
			   }
			   c = c->next;
			}
			// Print the password prompt only when it was not a command
			if ( ret == ITSDK_CONSOLE_NOTFOUND ) {
				_itsdk_console_printf("password:\r\n");
			}
		}
	} else {
		if (__console.pBuffer > 0) {
			// We are logged

			// Update session expiration
			uint64_t s = itsdk_time_get_ms()/1000;
			__console.expire = (uint32_t)s + ITSDK_CONSOLE_EXPIRE_S;

			// Process command
			itsdk_console_chain_t * c = &__console_head_chain;
			while ( c != NULL ) {
			   c->console_public((char*)__console.serialBuffer,(uint8_t)__console.pBuffer);
			   c->console_private((char*)__console.serialBuffer,(uint8_t)__console.pBuffer);
			   c = c->next;
			}
		}
	}

}

/**
 * Process 1 char read
 */
static void _itsdk_console_processChar(char c) {

	if ( c == '\n' || c == '\r' ) {
		_itsdk_console_processLine();
		__console.pBuffer = 0;
	} else {
		if ( __console.pBuffer < ITSDK_CONSOLE_LINEBUFFER ) {
			__console.serialBuffer[__console.pBuffer] = c;
			__console.pBuffer++;
		}
	}

}


// =================================================================================================
// Console operation chain management
// =================================================================================================


/**
 * Add an action to the chain, the action **must be** static
 * The action list is added at end of the chain
 */
void itsdk_console_registerCommand(itsdk_console_chain_t * chain) {
	itsdk_console_chain_t * c = &__console_head_chain;
	while ( c->next != NULL && c->next != chain ) {
	  c = c->next;
	}
	if ( c->next != chain ) {
		// the Action is not already existing
		c->next=chain;
		chain->next = NULL;
	}
}

/**
 * Remove an action to the chain, the action **must be** static
 */
void itsdk_console_removeCommand(itsdk_console_chain_t * chain) {
	itsdk_console_chain_t * c = &__console_head_chain;
	while ( c != NULL && c->next != chain ) {
	  c = c->next;
	}
	if ( c != NULL ) {
		c->next = c->next->next;
	}
}

/**
 * Search for an existing action
 */
bool itsdk_console_existCommand(itsdk_console_chain_t * chain) {
	itsdk_console_chain_t * c = &__console_head_chain;
	while ( c != NULL && c->next != chain ) {
	  c = c->next;
	}
	if ( c != NULL ) {
		return true;
	}
	return false;
}



#endif // ITSDK_WITH_CONSOLE

