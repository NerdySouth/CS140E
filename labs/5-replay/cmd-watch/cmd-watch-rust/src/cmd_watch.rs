use std::env::current_dir;
use std::fs;

pub fn  watch() -> Result<u32, std::io::Error> {
	//open dir
	//list direntries
	//check if any files havebeen modified since last mod time
	//if so, keep checking, and at end run command and wait on child
	loop {
		let curr_dir = current_dir()?;
		let curr_path = curr_dir.as_path();

		//get dir entriess
		let dir_entries = std::fs::read_dir(curr_path)?;
		let children: Vec<fs::DirEntry> = dir_entries.map(|&x| &x).collect();

	}
}