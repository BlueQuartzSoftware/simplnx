import os

# Set the directory path where your files are located
folder_path = 'C:/Users/Alex Jackson/Workspace/ITK_Doc_Images'

# Iterate over all files in the directory
for filename in os.listdir(folder_path):
    # Check if it's a file and not a directory
    if os.path.isfile(os.path.join(folder_path, filename)):
        # Check if the filename already starts with 'ITK'
        if not filename.startswith('ITK'):
            # Create the new filename by adding 'ITK' prefix
            new_filename = 'ITK' + filename
            
            # Construct the full file paths
            old_file_path = os.path.join(folder_path, filename)
            new_file_path = os.path.join(folder_path, new_filename)

            # Rename the file
            os.rename(old_file_path, new_file_path)
            print(f"Renamed {filename} to {new_filename}")
