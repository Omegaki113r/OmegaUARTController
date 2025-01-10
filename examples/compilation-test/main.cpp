/**
 * @file main.c
 * @author Omegaki113r
 * @date Thursday, 17th October 2024 7:03:04 pm
 * @copyright Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * */
/*
 * Project: OmegaUARTController
 * File Name: main.c
 * File Created: Thursday, 17th October 2024 7:03:04 pm
 * Author: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Last Modified: Thursday, 17th October 2024 7:19:46 pm
 * Modified By: Omegaki113r (omegaki113r@gmail.com)
 * -----
 * Copyright 2024 - 2024 0m3g4ki113r, Xtronic
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 */
#include <stdio.h>

#include <OmegaUARTController.hpp>

OmegaUARTHandle handle = 1000;

void main() {
	OmegaUARTController_init(nullptr);
	printf("hello world %lld\n",handle);
}