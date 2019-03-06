static int
MOCK_ssl_do_connect (int unknown_a, int unknown_b)
{
	foo_1();

	if (bar_1())
	{
		if (!bar_2())
		{
			int i;

			foo_2();

			for (i = 0; unknown_a; i++)
			{
				baz_1();
			}

			foo_3();

			for (i = 0; unknown_b; i++)
			{
				baz_2();
			}

			foo_4();
		} 
		else {
			foo_5();
			goto conn_fail;
		}

		foo_6();

		switch (bar_3())
		{
		case 0:
			{
				foo_7();
				if (bar_4())
				{
					foo_8();
					if (bar_5())
						foo_9();
					else
						goto conn_fail;
				}
				break;
			}
		case 20:
		case 21:
		case 18:
		case 19:
		case 10:
			if (bar_6())
			{
				foo_10();
				break;
			}
		default:
			foo_11();
conn_fail:

			foo_12();

			return 0;
		}

		foo_13();

		return 1;	
	} else {

		foo_14();

		if (bar_6())
		{
			foo_15();
			return 2;
		}

		foo_16();

		return 3;
	}
}