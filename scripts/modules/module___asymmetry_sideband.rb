#!/usr/bin/env ruby
require 'yaml'
require 'fileutils'
require 'optparse'

options = { slurm: false, deps: nil }

OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [--slurm] [--dependency afterok:IDs] PROJECT_NAME"

  opts.on('--slurm', 'Submit via sbatch') do
    options[:slurm] = true
  end

  # Fix: accept the dependency string into a block variable `d`
  opts.on('--dependency D', 'SBATCH --dependency string (e.g. afterok:1234)') do |d|
    options[:deps] = d
  end

  opts.on('-h', '--help', 'Show this help') do
    puts opts
    exit
  end
end.order!

project_name = ARGV.shift or abort "ERROR: PROJECT_NAME missing\n#{opts}"
out_root     = File.join("out", project_name)
abort "ERROR: out dir not found: #{out_root}" unless Dir.exist?(out_root)

# Default signal region:
signal_region = "M2>0.106&&M2<0.166"

# Background‐region variants to loop over:
background_regions = [
  "M2>0.2&&M2<0.45",
  "M2>0.2&&M2<0.4",
  "M2>0.2&&M2<0.35",
  "M2>0.2&&M2<0.3",
  "(M2>0&&M2<0.08)||(M2>0.2&&M2<0.4)"
]

job_ids = []

Dir.glob(File.join(out_root, "config_*")).sort.each do |config_dir|
  Dir.glob(File.join(config_dir, "**", "tree_info.yaml")).sort.each do |info_path|
    info       = YAML.load_file(info_path)
    orig_tfile = info.fetch("tfile")
    ttree      = info.fetch("ttree")

    leaf = File.dirname(info_path)
    pair = File.basename(File.dirname(leaf))

    # only run on real-data π⁰ combinations
    next unless File.exist?( File.join(leaf, File.basename(orig_tfile)) )
    next unless pair.include?("pi0")
    next if pair.start_with?("MC_")

    # collect each root invocation
    cmds = background_regions.map do |bkg|
      sanitized = bkg.
        gsub(/\s+/, '').
        gsub(/[^0-9A-Za-z.]/, '_').
        gsub(/_+/, '_').
        gsub(/^_|_$/, '')

      mod_out = File.join(leaf, "module-out___asymmetry_sideband_#{sanitized}")
      FileUtils.mkdir_p(mod_out)

      # build the ROOT command
      %Q{root -l -b -q 'src/modules/asymmetry.C("#{File.join(leaf, File.basename(orig_tfile))}","#{ttree}","#{pair}","#{mod_out}","#{signal_region}","#{bkg}")'}
    end

    if options[:slurm]
      # one sbatch script per <pair> running all background regions
      script = <<~SL
        #!/bin/bash
        #SBATCH --job-name=asym_sb_#{pair}
        #SBATCH --output=#{config_dir}/asym_#{pair}.%j.out
        #SBATCH --error=#{config_dir}/asym_#{pair}.%j.err
        #SBATCH --time=24:00:00
        #SBATCH --mem-per-cpu=4000
        #SBATCH --cpus-per-task=1
        #{"#SBATCH --dependency=#{options[:deps]}" if options[:deps]}

        cd #{Dir.pwd}

        #{cmds.join("\n\n        ")}
      SL

      sbatch_file = File.join(config_dir, "run_asym_#{pair}.slurm")
      File.write(sbatch_file, script)
      FileUtils.chmod('+x', sbatch_file)

      out = `sbatch #{sbatch_file}`
      if out =~ /Submitted batch job (\d+)/
        job_ids << $1
        puts "[SLURM_JOBS] #{job_ids.join(',')}"
      else
        warn "[module___asymmetry_sideband] sbatch failed: #{out}"
      end

    else
      # direct, sequential execution if not using SLURM
      cmds.each do |cmd|
        puts "[module___asymmetry_sideband] Running: #{cmd}"
        system(cmd) or warn "[module___asymmetry_sideband] ERROR for #{info_path}"
      end
    end
  end
end

if options[:slurm] && job_ids.any?
  puts "[SLURM_JOBS] #{job_ids.join(',')}"
end
