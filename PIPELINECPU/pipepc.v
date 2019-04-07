`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    15:46:40 10/25/2012 
// Design Name: 
// Module Name:    pipepc 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module pipepc(npc,clk,clrn,pc,LOADDEPEN
    );
	 input [31:0] npc;
	 input clk,clrn;
	 input LOADDEPEN;
	 output [31:0] pc;
	 dff32 program_counter(npc,clk,clrn,pc,LOADDEPEN);   //����32λ��D������ʵ��PC
endmodule
