static ncr_command_t mmb[] = {
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000200, 0x0001fff0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x0000020c, 0xc4c4c4c4, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000204, 0x00000000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000210, 0x0140c700, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000220, 0x014107f0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000214, 0x01410800, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000224, 0x01430f80, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000218, 0x01431000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000228, 0x01474c00, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x0000021c, 0x01475000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x0000022c, 0x01584000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002b0, 0x00000000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002d0, 0x00000000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002c0, 0x00000040, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002b4, 0x00000040, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002d4, 0x00000040, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002c4, 0x00000080, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002b8, 0x00000080, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002d8, 0x00000080, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002c8, 0x000000c0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002bc, 0x000000c0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002dc, 0x000000c0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002cc, 0x000000ff, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002e0, 0x00000010, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002e4, 0x00000050, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002e8, 0x00000090, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002ec, 0x000000d0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002f0, 0x00000030, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002f4, 0x00000070, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002f8, 0x000000b0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002fc, 0x000000ef, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000230, 0x0140c440, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000250, 0x0140c440, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000240, 0x0140c550, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000234, 0x0140c550, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000254, 0x0140c550, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000244, 0x0140c660, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000238, 0x0140c660, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000258, 0x0140c660, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000248, 0x0140c6b0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x0000023c, 0x0140c6b0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x0000025c, 0x0140c6b0, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x0000024c, 0x0140c700, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000280, 0x00000041, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000284, 0x00000041, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000288, 0x00000011, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x0000028c, 0x00000011, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000290, 0x00000010, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000294, 0x00000010, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000298, 0x00000210, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x0000029c, 0x00000010, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000002a0, 0x00000010, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x000001fc, 0x00000000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000200, 0x0001fff4, 0},
	{NCR_COMMAND_USLEEP, 0, 0, 10000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000200, 0x0001fffc, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000200, 0x0001fffe, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 0), 0x00000014, 0x00027fdf, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000030, 0xfffff000, 0},
	{NCR_COMMAND_WRITE, NCP_REGION_ID(21, 16), 0x00000034, 0x7fffffff, 0},
	{NCR_COMMAND_NULL, 0, 0, 0, 0}
};