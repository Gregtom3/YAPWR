#!/usr/bin/env ruby
require 'yaml'
require 'fileutils'

# module___asymmetryPW.rb
#
# Usage:
#   ruby module___asymmetryPW.rb PROJECT_NAME
#
# For each tree_info.yaml under out/PROJECT_NAME/config_*/…:
#  - derive filtered file path
#  - mkdir module-out___asymmetryPW
#  - call asymmetry macro with that outputDir

project_name = ARGV.fetch(0) {
  STDERR.puts "Usage: #{$0} PROJECT_NAME"
  exit 1
}

out_root = File.join("out", project_name)
unless Dir.exist?(out_root)
  STDERR.puts "ERROR: '#{out_root}' does not exist"
  exit 1
end

Dir.glob(File.join(out_root, "config_*")).sort.each do |config_dir|
  Dir.glob(File.join(config_dir, "**", "tree_info.yaml")).sort.each do |info_path|
    info       = YAML.load_file(info_path)
    orig_tfile = info.fetch("tfile")
    ttree      = info.fetch("ttree")

    leaf = File.dirname(info_path)
    pair = File.basename(File.dirname(leaf))
    filtered = File.join(leaf, File.basename(orig_tfile))
    unless File.exist?(filtered)
      STDERR.puts "[module___asymmetryPW] WARNING: no filtered file: #{filtered}"
      next
    end

    # make module output dir
    mod_out = File.join(leaf, "module-out___asymmetryPW")
    FileUtils.mkdir_p(mod_out)

    cmd = [
      "root", "-l", "-q",
      "src/asymmetry.C\\(\\\"#{filtered}\\\",\\\"#{ttree}\\\",\\\"#{pair}\\\",\\\"#{mod_out}\\\"\\)"
    ].join(' ')
    puts "[module___asymmetry] Running: #{cmd}"
    system(cmd) or
      STDERR.puts("[module___asymmetryPW] ERROR for #{info_path}")
  end
end
