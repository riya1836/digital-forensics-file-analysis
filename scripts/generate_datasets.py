import os

DATASETS = {
    "small": 1000,
    "medium": 5000,
    "large": 10000
}

EXTENSIONS = [
    ".txt",
    ".pdf",
    ".jpg",
    ".png",
    ".mp3",
    ".mp4",
    ".zip",
    ".c",
    ".py",
    ".doc"
]

# Different file sizes in bytes
FILE_SIZES = [
    1024,        # 1 KB
    5 * 1024,    # 5 KB
    10 * 1024,   # 10 KB
    50 * 1024,   # 50 KB
]


def create_file(path, size):
    """Create a deterministic synthetic file of the requested size."""
    pattern = b"DIGITAL_FORENSICS_TEST_DATA_"
    
    with open(path, "wb") as f:
        remaining = size

        while remaining > 0:
            chunk = pattern[:min(len(pattern), remaining)]
            f.write(chunk)
            remaining -= len(chunk)


def generate_dataset(name, count):
    dataset_dir = os.path.join("data", name)

    os.makedirs(dataset_dir, exist_ok=True)

    print(f"\nGenerating {name} dataset ({count} files)...")

    for i in range(count):
        extension = EXTENSIONS[i % len(EXTENSIONS)]
        size = FILE_SIZES[i % len(FILE_SIZES)]

        filename = f"sample_{i + 1:05d}{extension}"
        filepath = os.path.join(dataset_dir, filename)

        create_file(filepath, size)

        if (i + 1) % 1000 == 0:
            print(f"  Created {i + 1}/{count} files")

    print(f"Completed: {dataset_dir}")


def main():
    print("========================================")
    print(" Digital Forensics Dataset Generator")
    print("========================================")

    for name, count in DATASETS.items():
        generate_dataset(name, count)

    print("\n========================================")
    print("Dataset generation completed.")
    print("========================================")


if __name__ == "__main__":
    main()
