import os

def count_duplicates(arr):
    # 使用字典(dictionary)来存储元素和它们出现的次数
    count_dict = {}
    
    # 遍历数组中的所有元素，并统计它们的出现次数
    for num in arr:
        if num in count_dict:
            count_dict[num] += 1
        else:
            count_dict[num] = 1
    
    # 统计重复数字的个数
    duplicates = 0
    for count in count_dict.values():
        if count > 1:
            duplicates += 1
    
    return duplicates


if __name__ == '__main__':

    file = open("hash2.txt")

    hash = []
    for line in file.readlines():
        data = line.split(' ')
        hash.append(int(data[5]))
    print(hash[0])
    print(count_duplicates(hash))