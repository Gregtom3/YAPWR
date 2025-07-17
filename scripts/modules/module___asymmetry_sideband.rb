#!/usr/bin/env ruby
require 'yaml'
require 'fileutils'
require 'optparse'

options = { slurm: false }
OptionParser.new do |opts|
  opts.banner = "Usage: #{File.basename($0)} [--slurm] [--dependency afterok:IDs] PROJECT_NAME"
  opts.on('--slurm')                 { options[:slurm] = true }
  opts.on('--dependency DEPSTRING')  { options[:deps]  = DEPSTRING }
end.order!

project_name = ARGV.shift or abort "ERROR: PROJECT_NAME missing\n#{opts}"
out_root     = File.join("out", project_name)
abort "ERROR: out dir not found: #{out_root}" unless Dir.exist?(out_root)

# Default signal region:
signal_region = "M2>0.106 && M2<0.166"

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
    filtered = File.join(leaf, File.basename(orig_tfile))
    next unless File.exist?(filtered)
    next unless pair.include?("pi0")
    next if pair.start_with?("MC_")
    background_regions.each do |bkg|
      # sanitize region for directory and job-name
      sanitized = bkg.
        gsub(/\s+/, '').
        gsub(/[^0-9A-Za-z.]/, '_').
        gsub(/_+/, '_').
        gsub(/^_|_$/, '')

      mod_out = File.join(leaf, "module-out___asymmetry_sideband_#{sanitized}")
      FileUtils.mkdir_p(mod_out)
      root_cmd = [
          "root", "-l", "-b", "-q",
          %Q{'src/modules/asymmetry.C("#{filtered}","#{ttree}","#{pair}","#{mod_out}","#{signal_region}","#{bkg}")'}
      ].join(' ')

      if options[:slurm]
        # write sbatch script
        script = <<~SL
          #!/bin/bash
          #SBATCH --job-name=asym_sb_#{pair}_#{sanitized}
          #SBATCH --output=#{mod_out}/asym_#{pair}_#{sanitized}.out
          #SBATCH --error=#{mod_out}/asym_#{pair}_#{sanitized}.err
          #SBATCH --time=24:00:00
          #SBATCH --mem-per-cpu=4000
          #SBATCH --cpus-per-task=4
          #SBATCH --dependency=#{options[:deps]}
          cd #{Dir.pwd}
          #{root_cmd}
        SL

        sbatch_file = File.join(mod_out, "run_asym_#{pair}_#{sanitized}.slurm")
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
        # direct execution
        puts "[module___asymmetry_sideband] Running: #{root_cmd}"
        system(root_cmd) or warn "[module___asymmetry_sideband] ERROR for #{info_path}, region #{bkg}"
      end
    end
  end
end

if options[:slurm] && job_ids.any?
  puts "[SLURM_JOBS] #{job_ids.join(',')}"
end
