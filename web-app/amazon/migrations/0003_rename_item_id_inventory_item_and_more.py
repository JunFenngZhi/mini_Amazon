# Generated by Django 4.0.1 on 2022-04-18 01:10

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('amazon', '0002_rename_itemamount_inventory_item_amount_and_more'),
    ]

    operations = [
        migrations.RenameField(
            model_name='inventory',
            old_name='item_id',
            new_name='item',
        ),
        migrations.RenameField(
            model_name='order',
            old_name='item_id',
            new_name='item',
        ),
        migrations.AlterUniqueTogether(
            name='inventory',
            unique_together={('item', 'wh_id')},
        ),
    ]
