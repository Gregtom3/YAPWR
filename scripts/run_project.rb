#!/usr/bin/env ruby
require 'optparse'
require 'fileutils'
require 'yaml'

VOLATILE_PROJECT    = "pipi0-paper-v5-pass2"
VOLATILE_SUFFIX     = "*merged_cuts_noPmin*"
VOLATILE_TREE_NAME  = "dihadron_cuts_noPmin"

options = { append: false, maxEntries: nil }
OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [--append] [--maxEntries N] PROJECT_NAME CONFIG1 [CONFIG2 ...]"

  opts.on("--append", "Do not overwrite existing out/<PROJECT>; skip filterTree step") do
    options[:append] = true
  end

  opts.on("--maxEntries N", Integer, "Only copy first N entries (pass to filterTree)") do |n|
    options[:maxEntries] = n
  end

  opts.on("-h","--help","Prints this help") do
    puts opts
    exit
  end
end.parse!

if ARGV.size < 2
  STDERR.puts "ERROR: You must supply a PROJECT_NAME and at least one CONFIG file"
  STDERR.puts "Usage: #{$0} [--append] [--maxEntries N] PROJECT_NAME CONFIG1 [CONFIG2 ...]"
  exit 1
end

project_name = ARGV.shift
config_files  = ARGV

out_root = File.join("out", project_name)

unless options[:append]
  if Dir.exist?(out_root)
    print "Directory '#{out_root}' already exists. Overwrite everything? [y/N]: "
    ans = STDIN.gets.chomp.downcase
    unless %w[y yes].include?(ans)
      puts "Aborting."
      exit 0
    end
    FileUtils.rm_rf(out_root)
  end
end

base_dir    = File.join(
  "/volatile/clas12/users/gmat/clas12analysis.sidis.data",
  "clas12_dihadrons",
  "projects",
  VOLATILE_PROJECT,
  "data"
)
pattern      = File.join(base_dir, "*", VOLATILE_SUFFIX)
merged_files = Dir.glob(pattern)
if merged_files.empty?
  STDERR.puts "No merged_cuts files found under #{base_dir}"
  exit 1
end

config_files.each do |cfg|
  next unless File.file?(cfg)
  name = File.basename(cfg, File.extname(cfg))
  outc = File.join(out_root, "config_#{name}")

  FileUtils.mkdir_p(outc)
  FileUtils.cp(cfg, outc) unless options[:append] && Dir.exist?(outc)
  File.write(File.join(outc, "volatile_project.txt"), VOLATILE_PROJECT)

  merged_files.each do |fp|
    pair = File.basename(File.dirname(fp))
    tag  = File.basename(fp).split('_merged_cuts_noPmin').first
    td   = File.join(outc, pair, tag)
    FileUtils.mkdir_p(td)

    info = {
      'tfile' => File.absolute_path(fp),
      'ttree' => VOLATILE_TREE_NAME
    }
    File.write(File.join(td, "tree_info.yaml"), info.to_yaml)
  end

  puts "Set up: #{outc}"
end

puts "\nDirectory structure under #{out_root}:"
system("tree", out_root)

unless options[:append]
  args = [project_name]
  args << options[:maxEntries].to_s if options[:maxEntries]
  puts "\n=> Invoking module___filterTree.rb: PROJECT=#{project_name} maxEntries=#{options[:maxEntries]||'all'}"
  unless system("ruby", "./scripts/modules/module___filterTree.rb", *args)
    STDERR.puts "ERROR: module___filterTree.rb failed"
    exit 1
  end
end
