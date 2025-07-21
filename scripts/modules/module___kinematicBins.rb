#!/usr/bin/env ruby
# ------------------------------------------------------------------
#  module___kinematicBins.rb   – run kinematicBins.C on every
#                                filtered tree in out/<PROJECT>
# ------------------------------------------------------------------
require 'yaml'
require 'fileutils'

project = ARGV.shift or abort "usage: ruby #{$0} <PROJECT_NAME>"
out_root = File.join("out", project)
abort "ERROR: #{out_root} not found" unless Dir.exist?(out_root)

Dir.glob(File.join(out_root, "config_*")).sort.each do |cfg|
  Dir.glob(File.join(cfg, "**", "tree_info.yaml")).sort.each do |tinfo|
    info   = YAML.load_file(tinfo)
    ttree  = info['ttree']
    src    = info['tfile']
    leaf   = File.dirname(tinfo)
    froot  = File.join(leaf, File.basename(src))     # filtered file
    next unless File.exist?(froot)

    pair   = File.basename(File.dirname(leaf))
    outdir = File.join(leaf, "module-out___kinematicBins")
    FileUtils.mkdir_p(outdir)
    # Ascend from leaf to the config_<NAME> dir
    cfg_dir   = leaf_dir
    cfg_dir   = File.dirname(cfg_dir) until File.basename(cfg_dir).start_with?("config_")
    cfg_name  = File.basename(cfg_dir).sub(/^config_/, '')
    primary_yaml = File.join(cfg_dir, "#{cfg_name}.yaml")
    macro  = %Q{src/modules/kinematicBins.C("#{src}","#{ttree}","#{pair}","#{primary_yaml}","#{outdir}")}
    cmd    = ["root", "-l", "-b", "-q", macro]       # each arg is atomic

    puts "[module___kinematicBins] #{cmd.join(' ')}"
    system(*cmd) or warn "[kinematicBins] ROOT exited with error for #{froot}"
  end
end
