#!/usr/bin/env ruby
require 'optparse'
require 'fileutils'
require 'yaml'

VOLATILE_PROJECT    = "pipi0-paper-v5-pass2"
VOLATILE_SUFFIX     = "*merged_cuts_noPmin*"
VOLATILE_TREE_NAME  = "dihadron_cuts_noPmin"

options = { append: false }
OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [--append] PROJECT_NAME CONFIG1 [CONFIG2 ...]"

  opts.on("--append", "Do not overwrite existing out/<PROJECT>; skip filterTree step") do
    options[:append] = true
  end

  opts.on("-h", "--help", "Prints this help") do
    puts opts
    exit
  end
end.parse!

if ARGV.size < 2
  STDERR.puts "ERROR: You must supply a PROJECT_NAME and at least one CONFIG file"
  STDERR.puts "Usage: #{$0} [--append] PROJECT_NAME CONFIG1 [CONFIG2 ...]"
  exit 1
end

project_name = ARGV.shift
config_files  = ARGV

# top‐level output dir
out_root = File.join("out", project_name)

unless options[:append]
  # prompt overwrite if out/<PROJECT_NAME> exists
  if Dir.exist?(out_root)
    print "Directory '#{out_root}' already exists. Overwrite everything? [y/N]: "
    answer = STDIN.gets.chomp.downcase
    unless %w[y yes].include?(answer)
      puts "Aborting."
      exit 0
    end
    FileUtils.rm_rf(out_root)
  end
end

# find all merged‐cuts files under the volatile project
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

# process each config: create dirs, copy configs, write tree_info.yaml
config_files.each do |config_path|
  next unless File.file?(config_path)

  config_name = File.basename(config_path, File.extname(config_path))
  out_base    = File.join(out_root, "config_#{config_name}")

  FileUtils.mkdir_p(out_base)
  FileUtils.cp(config_path, out_base) unless options[:append] && Dir.exist?(out_base)
  File.write(File.join(out_base, "volatile_project.txt"), VOLATILE_PROJECT)

  merged_files.each do |filepath|
    pair = File.basename(File.dirname(filepath))
    tag  = File.basename(filepath).split('_merged_cuts_noPmin').first
    target_dir = File.join(out_base, pair, tag)
    FileUtils.mkdir_p(target_dir)

    info = { 'tfile' => File.absolute_path(filepath),
             'ttree' => VOLATILE_TREE_NAME }
    File.write(File.join(target_dir, "tree_info.yaml"), info.to_yaml)
  end

  puts "Set up: #{out_base}"
end

# print the directory structure
puts "\nDirectory structure under #{out_root}:"
system("tree", out_root)

unless options[:append]
  puts "\nInvoking module___filterTree.rb for project '#{project_name}'..."
  unless system("ruby", "./scripts/modules/module___filterTree.rb", project_name)
    STDERR.puts "ERROR: module___filterTree.rb failed"
    exit 1
  end
end
