require 'set'

total_options = %W[
	MOST_ANCESTORS
	LEAST_ANCESTORS
	MOST_DESCENDANTS
	LEAST_DESCENDANTS
	MOST_DIRECT_DESCENDANTS
	LEAST_DIRECT_DESCENDANTS
	MOST_FEEDBACK
	LEAST_FEEDBACK
	MOST_RECENT
	LEAST_RECENT
	RAND
]

data_groups = {}
totals = {}
Dir.glob("corpus_tests/*.log").each do |file|
  total = 0.0
  count = 0
  data_name = File.basename(file).gsub(".log", "").gsub("_RAND", "__RAND").gsub("_MOST", "__MOST").gsub("_LEAST", "__LEAST")
  data_name = Set.new(data_name.split("__")).to_a.sort.join("-")
  data_groups[data_name] ||= []
  File.read(file).split("\n").each do |line|
    next unless line.include?("Feedback:")
    datas = line.split(" | ")
    iters = datas[0].gsub("Iters:", "").strip
    feedback = datas[4].gsub("Feedback:", "").gsub("edges", "").strip

    if iters == "200000000"
      totals[data_name] ||= { "num": 0, "total": 0 }
      totals[data_name][:total] += feedback.to_i
      totals[data_name][:num] += 1
    end

    data_groups[data_name].push([
      iters.to_i,
      datas[1].gsub("iters/s", "").strip.to_f,
      datas[2].gsub("Crashes:", "").strip.to_i,
      datas[3].gsub("Corpus:", "").strip.to_i,
      feedback.to_i,
      datas[5].gsub("s", "").strip.to_f,
    ])
  end

  data_groups[data_name].sort_by! { |items| items[0] }
end

ordered = totals.to_a.sort_by { |group_name, info|
  info[:avg] = info[:total] / info[:num].to_f
  info[:avg]
}.reverse

def save_group(f, group_name, info, data_groups)
  data = data_groups[group_name]
  group_avg = info[:total] / info[:num].to_f
  f.write("\"#{group_name} #{group_avg.round(1)} avg (n=#{info[:num]})\"\n")
  f.write(data.map{|items|items.map(&:to_s).join(" ")}.join("\n") + "\n")
  f.write("\n\n")
end

f = File.open("corpus_tests_data.txt", "wb")
save_group(f, "RAND", totals["RAND"], data_groups)
ordered.each do |group_name, info|
  next if group_name == "RAND"
  save_group(f, group_name, info, data_groups)
end
f.close()

all_names = Set.new()
all_names |= Set.new(total_options.combination(1).map{|names| names.sort.join("-")})
all_names |= Set.new(total_options.combination(2).map{|names| names.sort.join("-")})
all_names |= Set.new(total_options.combination(3).map{|names| names.sort.join("-")})

finished_names = Set.new(ordered.filter{|name,info| info[:num] >= 20}.map{|name,info| name})
remaining_names = all_names - finished_names
puts all_names - finished_names
puts "#{all_names.count} total"
puts "#{finished_names.count} finished"
puts "#{(all_names - finished_names).count} remaining"

if ARGV[0] == "run-test"
	remaining_names.each do |names|
    opts = names.split("-").map{|n| "-C #{n}"}
    if totals.has_key?(names)
      num = 20 - totals[names][:num]
    else
      num = 20
    end
    num.times do
      puts "TESTING #{names} #{num} more times"
      log_path = "corpus_tests/#{names.split("-").join("_")}.log"
      `
        rm -rf gen_test.resmack-state crashes ;
        LD_LIBRARY_PATH=build/release ./gen_test \
          -n 36 \
          -M 2 \
          -m 200000000 \
          -t 15000000 \
          -p 1 \
          #{opts.join(" ")} \
          -i 10000 >> "#{log_path}" ;
      `
    end
	end
end
