#!/usr/bin/ruby

buff = IO.read(ARGV[0]).split("\n").join('')
r = /EINA_OBJ_([A-Z]+)\(|GEL_APP_([A-Z]+)\(|eina_obj_get_([a-z]+)\(|gel_app_get_([a-z]+)\(|EINA_OBJ_GET_([A-Z]+)\(|GEL_APP_GET_([A-Z]+)\(/

ss = []
buff.scan(r).each do |m|
	ss << m.inject do |a,b| a or b end.downcase
end
puts ss.sort.uniq.join(",")
