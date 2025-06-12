#!/usr/bin/env ruby
# coding: utf-8
require 'optparse'
require 'fileutils'
require 'yaml'

VOLATILE_PROJECT   = "pipi0-paper-v5-pass2"
VOLATILE_SUFFIX    = "*merged_cuts_noPmin*"
VOLATILE_TREE_NAME = "dihadron_cuts_noPmin"

options = { append: false, maxEntries: nil, maxFiles: nil }
OptionParser.new do |opts|
  opts.banner = <<~USAGE
    Usage: #{$0} [--append] [--maxEntries N] [--maxFiles M] PROJECT_NAME RUNCARD CONFIG1 [CONFIG2 ...]
      --append           Do not overwrite existing out/<PROJECT>; skip filterTree step
      --maxEntries N     Only copy first N entries (pass to filterTree)
      --maxFiles M       Only generate M tree_info.yaml files per config
  USAGE

  opts.on("--append", "Do not overwrite existing out/<PROJECT>; skip filterTree step") do
    options[:append] = true
  end

  opts.on("--maxEntries N", Integer, "Only copy first N entries (pass to filterTree)") do |n|
    options[:maxEntries] = n
  end

  opts.on("--maxFiles M", Integer, "Only generate M tree_info.yaml files per config") do |m|
    options[:maxFiles] = m
  end

  opts.on("-h","--help","Prints this help") do
    puts opts
    exit
  end
end.parse!

if ARGV.size < 3
  STDERR.puts "ERROR: You must supply a PROJECT_NAME, RUNCARD and at least one CONFIG file"
  STDERR.puts "Usage: #{$0} [--append] [--maxEntries N] [--maxFiles M] PROJECT_NAME RUNCARD CONFIG1 [CONFIG2 ...]"
  exit 1
end

project_name = ARGV.shift

runcard_path = ARGV.shift
unless File.file?(runcard_path)
  STDERR.puts "ERROR: runcard not found: #{runcard_path}"
  exit 1
end

runcard = YAML.load_file(runcard_path)
modules = runcard.fetch('modules') {
  STDERR.puts "ERROR: runcard must define a top‐level 'modules' list"
  exit 1
}
unless modules.is_a?(Array) && modules.all?{|m| m.is_a?(String)}
  STDERR.puts "ERROR: 'modules' must be a list of module names"
  exit 1
end


config_files = ARGV

# === SETUP output directories & tree_info.yaml ===
out_root = File.join("out", project_name)
unless options[:append]
  if Dir.exist?(out_root)
    print "Directory '#{out_root}' exists. Overwrite? [y/N]: "
    ans = STDIN.gets.chomp.downcase
    exit unless %w[y yes].include?(ans)
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
  cfg_name = File.basename(cfg, File.extname(cfg))
  outc     = File.join(out_root, "config_#{cfg_name}")

  FileUtils.mkdir_p(outc)
  FileUtils.cp(cfg, outc) unless options[:append] && Dir.exist?(outc)
  File.write(File.join(outc, "volatile_project.txt"), VOLATILE_PROJECT)

  # skip pi0_pi0, then apply maxFiles
  valid = merged_files.reject { |fp| File.basename(File.dirname(fp)) == "pi0_pi0" }
  files = options[:maxFiles] && options[:maxFiles] > 0 ? valid.first(options[:maxFiles]) : valid

  files.each do |fp|
    pair = File.basename(File.dirname(fp))
    tag  = File.basename(fp).split('_merged_cuts_noPmin').first
    td   = File.join(outc, pair, tag)
    FileUtils.mkdir_p(td)
    info = { 'tfile' => File.absolute_path(fp), 'ttree' => VOLATILE_TREE_NAME }
    File.write(File.join(td, "tree_info.yaml"), info.to_yaml)
  end

  puts "Set up: #{outc} (wrote #{files.size} tree_info.yaml, skipped #{merged_files.size - valid.size} pi0_pi0)"
end

puts "\nDirectory structure under #{out_root}:"
system("tree", out_root)

# === RUN modules in user‐specified order ===
modules.each do |mod|
  case mod
  when 'filterTree'
    if options[:append]
      puts "Skipping filterTree (--append enabled)"
      next
    end
    args = [project_name]
    args << options[:maxEntries].to_s if options[:maxEntries]
    cmd = ["ruby", "./scripts/modules/module___filterTree.rb", *args]
    puts "\n=> filterTree: #{cmd.join(' ')}"
    system(*cmd) or abort("filterTree failed")

  when 'purityBinning'
    cmd = ["ruby", "./scripts/modules/module___purityBinning.rb", project_name]
    puts "\n=> purityBinning: #{cmd.join(' ')}"
    system(*cmd) or abort("purityBinning failed")

  when 'asymmetry'
    cmd = ["ruby", "./scripts/modules/module___asymmetry.rb", project_name]
    puts "\n=> asymmetry: #{cmd.join(' ')}"
    system(*cmd) or abort("asymmetry failed")

  when 'kinematicBins'
    cmd = ["ruby", "./scripts/modules/module___kinematicBins.rb", project_name]
    puts "\n=> kinematicBins: #{cmd.join(' ')}"
    system(*cmd) or abort("kinematicBins failed")

  else
    STDERR.puts "WARNING: unknown module '#{mod}' in runcard, skipping"
  end
end
