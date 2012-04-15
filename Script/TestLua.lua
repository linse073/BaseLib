printf = elib.printf
printf("load TestLua.lua successfully.")
a, b = elib.chello()
printf("c++ return:["..a.."]["..b.."].")

function luahello()
	return "hello from lua.", 2
end
